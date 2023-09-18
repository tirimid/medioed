#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>
#include <sys/ioctl.h>

#include "conf.h"

// padding size around line numbers.
#define GUTTER (CONF_GUTTER_LEFT + CONF_GUTTER_RIGHT)

static void exechighlight(struct frame const *f, struct highlight const *hl,
                          size_t redge, unsigned linumw);
static void drawline(struct frame const *f, unsigned *line, size_t *dcsr,
                     size_t redge, unsigned linumw);

struct frame
frame_create(char const *name, struct buf *buf)
{
	struct winsize tty_size;
	ioctl(0, TIOCGWINSZ, &tty_size);
	
	return (struct frame){
		.name = strdup(name),
		.pos_x = 0,
		.pos_y = 0,
		.size_x = tty_size.ws_col,
		.size_y = tty_size.ws_row,
		.buf = buf,
		.cursor = 0,
		.buf_start = 0,
	};
}

void
frame_destroy(struct frame *f)
{
	free(f->name);
}

void
frame_draw(struct frame const *f, bool active)
{
	// get linum properties required for rendering frame contents.
	unsigned bsx, bsy, bex, bey;
	unsigned linum_width = 0, right_edge;

	buf_pos(f->buf, f->buf_start, &bsx, &bsy);
	buf_pos(f->buf, f->buf->size, &bex, &bey);
	for (unsigned i = bsy + MIN(bey - bsy, f->size_y) + 1; i > 0; i /= 10)
		++linum_width;

	right_edge = f->size_x - GUTTER - linum_width;
	
	// clear frame.
	for (size_t i = 0; i < f->size_y; ++i) {
		for (size_t j = 0; j < f->size_x; ++j)
			mvaddch(f->pos_y + i, f->pos_x + j, ' ');
	}

	// write frame name and buffer modification indicator.
	for (unsigned i = 0; f->name[i] && i < f->size_x; ++i)
		mvaddch(f->pos_y, f->pos_x + i, f->name[i]);

	if (f->buf->modified) {
		char modind[] = CONF_BUFMOD_MARK "\0";
		if (f->size_x >= 0 && f->size_x < strlen(modind) + 1)
			modind[f->size_x] = 0;

		mvaddstr(f->pos_y, f->pos_x + f->size_x - strlen(modind), modind);
	}

	// set normal coloration.
	mvchgat(f->pos_y, f->pos_x, f->size_x, 0,
	        active ? conf_ghighlight : conf_gnorm, NULL);
	
	for (size_t i = 1; i < f->size_y; ++i)
		mvchgat(f->pos_y + i, f->pos_x, f->size_x, 0, conf_norm, NULL);

	// draw margins.
	for (size_t i = 0; i < conf_mtab_size; ++i) {
		struct margin const *m = &conf_mtab[i];

		if (m->pos >= f->size_x)
			continue;

		for (size_t i = 1; i < f->size_y; ++i) {
			unsigned drawx = GUTTER + linum_width + m->pos;
			mvaddch(f->pos_y + i, f->pos_x + drawx, m->ch);
			mvchgat(f->pos_y + i, f->pos_x + drawx, 1, 0, m->colpair, NULL);
		}
	}
	
	// write lines and linums.
	size_t drawcsr = f->buf_start;
	unsigned linum_ind = 0;
	
	for (unsigned i = 1; i < f->size_y; ++i) {
		if (linum_ind++ <= bey - bsy) {
			char drawtext[16];
			snprintf(drawtext, 16, "%u", bsy + linum_ind);
			unsigned drawx = CONF_GUTTER_LEFT + linum_width - strlen(drawtext);
			mvaddstr(f->pos_y + i, f->pos_x + drawx, drawtext);
		}

		drawline(f, &i, &drawcsr, right_edge, linum_width);
	}

	// set highlight coloration.
	for (size_t i = 0; i < conf_htab_size; ++i)
		exechighlight(f, &conf_htab[i], right_edge, linum_width);
	
	// set cursor coloration.
	unsigned csrx, csry;
	frame_pos(f, f->cursor, &csrx, &csry);
	mvchgat(f->pos_y + csry, f->pos_x + csrx, 1, 0, conf_cursor, NULL);

	// set linum coloration.
	for (size_t i = 1; i < f->size_y; ++i) {
		mvchgat(f->pos_y + i, f->pos_x, GUTTER + linum_width, 0, conf_linum,
		        NULL);
	}
}

void
frame_pos(struct frame const *f, size_t pos, unsigned *out_x, unsigned *out_y)
{
	unsigned bsx, bsy, bex, bey;
	unsigned linum_width = 0, right_edge;

	buf_pos(f->buf, f->buf_start, &bsx, &bsy);
	buf_pos(f->buf, f->buf->size, &bex, &bey);
	for (unsigned i = bsy + MIN(bey - bsy, f->size_y) + 1; i > 0; i /= 10)
		++linum_width;

	right_edge = f->size_x - GUTTER - linum_width;
	
	*out_x = 0;
	*out_y = 1;
	
	for (size_t i = f->buf_start; i < pos; ++i) {
		if (f->buf->conts[i] == '\t')
			*out_x += CONF_TABSIZE - *out_x % CONF_TABSIZE - 1;
		
		if (f->buf->conts[i] == '\n' || *out_x >= right_edge - 1) {
			*out_x = 0;
			++*out_y;
			continue;
		}

		++*out_x;
	}

	*out_x += GUTTER + linum_width;
}

void
frame_move_cursor(struct frame *f, unsigned x, unsigned y)
{
	size_t *csr = &f->cursor, *bs = &f->buf_start;
	char const *bconts = f->buf->conts;

	// adjust the actual cursor position.
	*csr = 0;
	
	for (; *csr < f->buf->size && y > 0; ++*csr) {
		if (bconts[*csr] == '\n')
			--y;
	}

	for (; *csr < f->buf->size && bconts[*csr] != '\n' && x > 0; --x)
		++*csr;

	// redetermine buffer boundaries for rendering.
	unsigned bsx, bsy, csrx, csry;
	
	frame_pos(f, *bs, &bsx, &bsy);
	frame_pos(f, f->cursor, &csrx, &csry);
	
	while (csry >= bsy + f->size_y - 1) {
		for (++*bs; *bs < f->buf->size && bconts[*bs - 1] != '\n'; ++*bs);
		
		frame_pos(f, *bs, &bsx, &bsy);
		frame_pos(f, f->cursor, &csrx, &csry);
	}

	buf_pos(f->buf, *bs, &bsx, &bsy);
	buf_pos(f->buf, f->cursor, &csrx, &csry);
	
	while (csry < bsy) {
		for (--*bs; *bs > 0 && bconts[*bs - 1] != '\n'; --*bs);
		
		buf_pos(f->buf, *bs, &bsx, &bsy);
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

static void
exechighlight(struct frame const *f, struct highlight const *hl, size_t redge,
              unsigned linumw)
{
	pcre2_match_data *md = pcre2_match_data_create_from_pattern(hl->re, NULL);
	PCRE2_SIZE off = f->buf_start, len = f->buf->size;
	PCRE2_SPTR sub = (PCRE2_SPTR)f->buf->conts;

	while (pcre2_match(hl->re, sub, len, off, 0, md, NULL) >= 0) {
		PCRE2_SIZE *offv = pcre2_get_ovector_pointer(md);
		
		unsigned hlx, hly;
		frame_pos(f, offv[0], &hlx, &hly);
		if (hly >= f->size_y)
			break;

		unsigned draw_x = hlx - GUTTER - linumw, draw_y = hly;
		for (size_t i = offv[0]; i < offv[1]; ++i) {
			char ch = f->buf->conts[i];
			
			if (draw_x >= redge) {
				draw_x = 0;
				++draw_y;
			}

			if (draw_y >= f->size_y)
				break;

			switch (ch) {
			case '\n':
				draw_x = 0;
				++draw_y;
				break;
			case '\t': {
				unsigned tabrem = CONF_TABSIZE - draw_x % CONF_TABSIZE;
				mvchgat(f->pos_y + draw_y, f->pos_x + GUTTER + linumw + draw_x,
				        tabrem, 0, hl->colpair, NULL);
				draw_x += tabrem;
				break;
			}
			default:
				mvchgat(f->pos_y + draw_y, f->pos_x + GUTTER + linumw + draw_x,
				        1, 0, hl->colpair, NULL);
				++draw_x;
				break;
			}
		}
		
		off = offv[1];
	}

	pcre2_match_data_free(md);
}

static void
drawline(struct frame const *f, unsigned *line, size_t *dcsr, size_t redge,
         unsigned linumw)
{
	char const *bconts = f->buf->conts;
	size_t bsize = f->buf->size;
	unsigned i;

	for (i = 0; *dcsr < bsize && bconts[*dcsr] != '\n'; ++i, ++*dcsr) {
		char ch = bconts[*dcsr];

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
			mvaddch(f->pos_y + *line, f->pos_x + GUTTER + linumw + i, ch);
			break;
		}
	}

	if (i >= redge)
		++*line;

	++*dcsr;
}
