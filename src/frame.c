#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>
#include <sys/ioctl.h>

#include "conf.h"

// padding size around line numbers.
#define GUTTER (CONF_GUTTER_LEFT + CONF_GUTTER_RIGHT)

static unsigned getlinumw(struct frame const *f);
static void exechighlight(struct frame const *f, struct highlight const *hl);
static void drawline(struct frame const *f, unsigned *line, size_t *dcsr);

struct frame
frame_create(char const *name, struct buf *buf)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);
	
	return (struct frame){
		.name = strdup(name),
		.pos_x = 0,
		.pos_y = 0,
		.size_x = ws.ws_col,
		.size_y = ws.ws_row,
		.buf = buf,
		.cursor = 0,
		.buf_start = 0,
		.localmode = strdup("\0"),
	};
}

void
frame_destroy(struct frame *f)
{
	free(f->name);
	free(f->localmode);
}

void
frame_draw(struct frame const *f, bool active)
{
	unsigned bsx, bsy, bex, bey;
	buf_pos(f->buf, f->buf_start, &bsx, &bsy);
	buf_pos(f->buf, f->buf->size, &bex, &bey);
	
	unsigned linumw = getlinumw(f);
	unsigned ledge = GUTTER + linumw;
	
	// write frame name and buffer modification marker.
	for (unsigned i = 0; f->name[i] && i < f->size_x; ++i)
		mvaddch(f->pos_y, f->pos_x + i, f->name[i]);

	if (f->buf->modified) {
		char modmk[] = CONF_BUFMOD_MARK "\0";
		if (f->size_x >= 0 && f->size_x < strlen(modmk) + 1)
			modmk[f->size_x] = 0;
		
		mvaddstr(f->pos_y, f->pos_x + f->size_x - strlen(modmk), modmk);
	}

	// set normal coloration.
	mvchgat(f->pos_y, f->pos_x, f->size_x, 0, active ? conf_ghighlight : conf_gnorm, NULL);
	for (size_t i = 1; i < f->size_y; ++i)
		mvchgat(f->pos_y + i, f->pos_x, f->size_x, 0, conf_norm, NULL);

	// draw margins.
	for (size_t i = 0; i < conf_mtab_size; ++i) {
		if (ledge + conf_mtab[i].pos >= f->size_x)
			continue;

		for (unsigned j = 1; j < f->size_y; ++j) {
			unsigned x = f->pos_x + ledge + conf_mtab[i].pos, y = f->pos_y + j;
			mvaddch(y, x, conf_mtab[i].ch);
			mvchgat(y, x, 1, 0, conf_mtab[i].colpair, NULL);
		}
	}

	// write lines and linums.
	size_t drawcsr = f->buf_start;
	unsigned linum_ind = 0;
	for (unsigned i = 1; i < f->size_y; ++i) {
		if (linum_ind++ <= bey - bsy) {
			char drawtext[16];
			snprintf(drawtext, 16, "%u", bsy + linum_ind);
			mvaddstr(f->pos_y + i, f->pos_x + CONF_GUTTER_LEFT + linumw - strlen(drawtext), drawtext);
		}

		drawline(f, &i, &drawcsr);
	}

	// set highlight coloration.
	for (size_t i = 0; i < conf_htab_size; ++i)
		exechighlight(f, &conf_htab[i]);
	
	// set cursor coloration.
	unsigned csrx, csry;
	frame_pos(f, f->cursor, &csrx, &csry);
	mvchgat(f->pos_y + csry, f->pos_x + csrx, 1, 0, conf_cursor, NULL);

	// set linum coloration.
	for (size_t i = 1; i < f->size_y; ++i)
		mvchgat(f->pos_y + i, f->pos_x, ledge, 0, conf_linum, NULL);
}

void
frame_pos(struct frame const *f, size_t pos, unsigned *out_x, unsigned *out_y)
{
	unsigned linumw = getlinumw(f);
	unsigned redge = f->size_x - GUTTER - linumw;
	
	*out_x = 0;
	*out_y = 1;
	
	for (size_t i = f->buf_start; i < pos; ++i) {
		if (f->buf->conts[i] == '\t')
			*out_x += CONF_TABSIZE - *out_x % CONF_TABSIZE - 1;
		
		if (f->buf->conts[i] == '\n' || *out_x >= redge - 1) {
			*out_x = 0;
			++*out_y;
			continue;
		}

		++*out_x;
	}

	*out_x += GUTTER + linumw;
}

void
frame_move_cursor(struct frame *f, unsigned x, unsigned y)
{
	// adjust the actual cursor position.
	f->cursor = 0;
	
	for (; f->cursor < f->buf->size && y > 0; ++f->cursor) {
		if (f->buf->conts[f->cursor] == '\n')
			--y;
	}

	for (; f->cursor < f->buf->size && f->buf->conts[f->cursor] != '\n' && x > 0; --x)
		++f->cursor;

	// redetermine buffer boundaries for rendering.
	unsigned bsx, bsy, csrx, csry;
	
	frame_pos(f, f->buf_start, &bsx, &bsy);
	frame_pos(f, f->cursor, &csrx, &csry);
	
	while (csry >= bsy + f->size_y - 1) {
		++f->buf_start;
		while (f->buf_start < f->buf->size && f->buf->conts[f->buf_start - 1] != '\n')
			++f->buf_start;
		
		frame_pos(f, f->buf_start, &bsx, &bsy);
		frame_pos(f, f->cursor, &csrx, &csry);
	}

	buf_pos(f->buf, f->buf_start, &bsx, &bsy);
	buf_pos(f->buf, f->cursor, &csrx, &csry);
	
	while (csry < bsy) {
		--f->buf_start;
		while (f->buf_start > 0 && f->buf->conts[f->buf_start - 1] != '\n')
			--f->buf_start;
		
		buf_pos(f->buf, f->buf_start, &bsx, &bsy);
		buf_pos(f->buf, f->cursor, &csrx, &csry);
	}
}

void
frame_relmove_cursor(struct frame *f, int x, int y, bool lwrap)
{
	if (lwrap) {
		int dir = SIGN(x);
		long bs_dst = -(long)f->cursor;
		long be_dst = f->buf->size - f->cursor;
		
		x = dir == -1 ? MAX(x, bs_dst) : MIN(x, be_dst);
		
		for (; f->cursor >= 0 && f->cursor <= f->buf->size && x != 0; x -= dir)
			f->cursor += dir;
	}

	unsigned csrx, csry;

	buf_pos(f->buf, f->cursor, &csrx, &csry);
	csrx = (long)csrx + x < 0 ? 0 : csrx + x;
	csry = (long)csry + y < 0 ? 0 : csry + y;
	
	frame_move_cursor(f, csrx, csry);
}

static unsigned
getlinumw(struct frame const *f)
{
	unsigned bsx, bsy, bex, bey;
	buf_pos(f->buf, f->buf_start, &bsx, &bsy);
	buf_pos(f->buf, f->buf->size, &bex, &bey);

	unsigned linumw = 0;
	for (unsigned i = bsy + MIN(bey - bsy, f->size_y) + 1; i > 0; i /= 10)
		++linumw;

	return linumw;
}

static void
exechighlight(struct frame const *f, struct highlight const *hl)
{
	if (strcmp(hl->localmode, f->localmode))
		return;
	
	pcre2_match_data *md = pcre2_match_data_create_from_pattern(hl->re, NULL);
	PCRE2_SIZE off = f->buf_start;
	unsigned linumw = getlinumw(f), ledge = GUTTER + linumw;
	while (pcre2_match(hl->re, (PCRE2_SPTR)f->buf->conts, f->buf->size, off, 0, md, NULL) >= 0) {
		PCRE2_SIZE *offv = pcre2_get_ovector_pointer(md);
		
		unsigned hlx, hly;
		frame_pos(f, offv[0], &hlx, &hly);
		if (hly >= f->size_y)
			break;

		unsigned x = hlx - ledge, y = hly;
		for (size_t i = offv[0]; i < offv[1]; ++i) {
			if (x >= f->size_x - ledge) {
				x = 0;
				++y;
			}

			if (y >= f->size_y)
				break;

			unsigned w;
	
			switch (f->buf->conts[i]) {
			case '\n':
				x = 0;
				++y;
				continue;
			case '\t':
				w = CONF_TABSIZE - x % CONF_TABSIZE;
				break;
			default:
				w = 1;
				break;
			}

			mvchgat(f->pos_y + y, f->pos_x + ledge + x, w, hl->attr, hl->colpair, NULL);
			x += w;
		}
		
		off = offv[1];
	}

	pcre2_match_data_free(md);
}

static void
drawline(struct frame const *f, unsigned *line, size_t *dcsr)
{
	unsigned linumw = getlinumw(f), ledge = GUTTER + linumw, redge = f->size_x - ledge;
	unsigned i;

	for (i = 0; *dcsr < f->buf->size && f->buf->conts[*dcsr] != '\n'; ++i, ++*dcsr) {
		char ch = f->buf->conts[*dcsr];

		if (i >= redge) {
			i = 0;
			++*line;
		}

		if (*line >= f->size_y)
			break;
		
		switch (ch) {
		case '\t':
			i += CONF_TABSIZE - i % CONF_TABSIZE - 1;
			break;
		default:
			mvaddch(f->pos_y + *line, f->pos_x + ledge + i, ch);
			break;
		}
	}

	if (i >= redge)
		++*line;

	++*dcsr;
}
