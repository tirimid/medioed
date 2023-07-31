#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>
#include <sys/ioctl.h>

#include "def.h"

// padding size around line numbers.
#define GUTTER_LEFT 1
#define GUTTER_RIGHT 1
#define GUTTER (GUTTER_LEFT + GUTTER_RIGHT)

static void draw_line(struct frame const *f, unsigned *line, size_t *dcsr,
                      size_t redge, unsigned linumw);

struct frame_theme
frame_theme_default(void)
{
	struct frame_theme ft = {
		.norm_fg = COLOR_WHITE,
		.norm_bg = COLOR_BLACK,
		.cursor_fg = COLOR_BLACK,
		.cursor_bg = COLOR_WHITE,
		.linum_fg = COLOR_YELLOW,
		.linum_bg = COLOR_BLACK,
		.highlights = arraylist_create(),
		.tabsize = 4,
	};

	ft.norm_pair = alloc_pair(ft.norm_fg, ft.norm_bg);
	ft.cursor_pair = alloc_pair(ft.cursor_fg, ft.cursor_bg);
	ft.linum_pair = alloc_pair(ft.linum_fg, ft.linum_bg);

	return ft;
}

struct frame_theme
frame_theme_load(char const *theme_path)
{
	// TODO: add custom theme loading from file.
	return frame_theme_default();
}

void
frame_theme_destroy(struct frame_theme *ft)
{
	free_pair(ft->norm_pair);
	free_pair(ft->cursor_pair);
	free_pair(ft->linum_pair);
	
	for (size_t i = 0; i < ft->highlights.size; ++i) {
		struct frame_theme_highlight *highlight = ft->highlights.data[i];
		
		free(highlight->regex);
		free_pair(highlight->pair);
	}

	arraylist_destroy(&ft->highlights);
}

struct frame
frame_create(char const *name, struct buf *buf, struct frame_theme const *theme)
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
		.theme = theme,
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

	// write frame name.
	for (unsigned i = 0; f->name[i] && i < f->size_x; ++i)
		mvaddch(f->pos_y, f->pos_x + i, f->name[i]);
	
	// write lines and linums.
	size_t drawcsr = f->buf_start;
	unsigned linum_ind = 0;
	
	for (unsigned i = 1; i < f->size_y; ++i) {
		if (linum_ind++ <= bey - bsy) {
			char drawtext[16];
			snprintf(drawtext, 16, "%u", bsy + linum_ind);
			unsigned drawpos = GUTTER_LEFT + linum_width - strlen(drawtext);
			mvaddstr(f->pos_y + i, f->pos_x + drawpos, drawtext);
		}

		draw_line(f, &i, &drawcsr, right_edge, linum_width);
	}

	struct frame_theme const *ft = f->theme;

	// set normal coloration.
	mvchgat(f->pos_y, f->pos_x, f->size_x, 0,
	        active ? GLOBAL_HIGHLIGHT_PAIR : GLOBAL_NORM_PAIR, NULL);
	
	for (size_t i = 1; i < f->size_y; ++i)
		mvchgat(f->pos_y + i, f->pos_x, f->size_x, 0, ft->norm_pair, NULL);

	// set cursor coloration.
	unsigned csrx, csry;
	frame_pos(f, f->cursor, &csrx, &csry);
	mvchgat(f->pos_y + csry, f->pos_x + csrx, 1, 0, ft->cursor_pair, NULL);

	// set linum coloration.
	for (size_t i = 1; i < f->size_y; ++i) {
		mvchgat(f->pos_y + i, f->pos_x, GUTTER + linum_width, 0, ft->linum_pair,
		        NULL);
	}

	// TODO: set highlight coloration.
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
			*out_x += f->theme->tabsize - *out_x % f->theme->tabsize - 1;
		
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
draw_line(struct frame const *f, unsigned *line, size_t *dcsr, size_t redge,
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
			i += f->theme->tabsize - i % f->theme->tabsize - 1;
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
