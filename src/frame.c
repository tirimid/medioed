#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>

// padding size around line numbers.
#define GUTTER_LEFT 1
#define GUTTER_RIGHT 1
#define GUTTER (GUTTER_LEFT + GUTTER_RIGHT)

struct frame_theme
frame_theme_default(void)
{
	return (struct frame_theme){
		.norm_fg = COLOR_WHITE,
		.norm_bg = COLOR_BLACK,
		.cursor_fg = COLOR_BLACK,
		.cursor_bg = COLOR_WHITE,
		.linum_fg = COLOR_YELLOW,
		.linum_bg = COLOR_BLUE,
		.highlights = arraylist_create(),
		.tabsize = 4,
	};
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
	for (size_t i = 0; i < ft->highlights.size; ++i)
		free(((struct frame_theme_highlight *)ft->highlights.data[i])->regex);
	
	arraylist_destroy(&ft->highlights);
}

struct frame
frame_create(char const *name, unsigned px, unsigned py, unsigned sx,
             unsigned sy, struct buf *buf, struct frame_theme const *theme)
{
	return (struct frame){
		.name = strdup(name),
		.pos_x = px,
		.pos_y = py,
		.size_x = sx,
		.size_y = sy,
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
frame_draw(struct frame const *f)
{
	// get linum properties required for rendering frame contents.
	unsigned bsx, bsy;
	unsigned linum_width = 0;

	buf_pos(f->buf, f->buf_start, &bsx, &bsy);
	for (unsigned bey = bsy + f->size_y; bey > 0; bey /= 10)
		++linum_width;
	
	// clear frame.
	for (size_t i = 0; i < f->size_y; ++i) {
		for (size_t j = 0; j < f->size_x; ++j)
			mvaddch(f->pos_y + i, f->pos_x + j, ' ');
	}
	
	// write frame content characters.
	size_t drawcsr = f->buf_start;
	for (size_t i = 0; i < f->size_y; ++i) {
		for (size_t j = 0; j < f->size_x - GUTTER - linum_width; ++j) {
			char ch = drawcsr >= f->buf->size ? ' ' : f->buf->conts[drawcsr++];
			
			switch (ch) {
			case '\t':
				j += f->theme->tabsize - j % f->theme->tabsize - 1;
				break;
			case '\n':
				goto skip_x;
			case '\r':
				--j;
				break;
			default:
				mvaddch(f->pos_y + i, f->pos_x + GUTTER + linum_width + j, ch);
				break;
			}
		}
	skip_x:;
	}

	// write linum characters.
	for (unsigned i = 0; i < f->size_y; ++i) {
		char linum[16];
		snprintf(linum, 16, "%u", bsy + i + 1);
		mvaddstr(f->pos_y + i, f->pos_x + GUTTER_LEFT, linum);
	}
	
	// set normal coloration.
	init_pair(1, f->theme->norm_fg, f->theme->norm_bg);
	for (size_t i = 0; i < f->size_y; ++i)
		mvchgat(f->pos_y + i, f->pos_x, f->size_x, 0, 1, NULL);

	// set cursor coloration.
	unsigned csrx, csry;
	frame_cursor_pos(f, &csrx, &csry);
	init_pair(2, f->theme->cursor_fg, f->theme->cursor_bg);
	mvchgat(f->pos_y + csry, f->pos_x + csrx, 1, 0, 2, NULL);

	// set linum coloration.
	init_pair(3, f->theme->linum_fg, f->theme->linum_bg);
	for (size_t i = 0; i < f->size_y; ++i)
		mvchgat(f->pos_y + i, f->pos_x, GUTTER + linum_width, 0, 3, NULL);

	// TODO: set highlight coloration.
}

void
frame_cursor_pos(struct frame const *f, unsigned *out_x, unsigned *out_y)
{
	unsigned bsx, bsy;
	unsigned linum_width = 0;

	buf_pos(f->buf, f->buf_start, &bsx, &bsy);
	for (unsigned bey = bsy + f->size_y; bey > 0; bey /= 10)
		++linum_width;
	
	*out_x = GUTTER + linum_width;
	*out_y = 0;
	for (size_t i = f->buf_start; i < f->cursor; ++i) {
		++*out_x;

		if (f->buf->conts[i] == '\t')
			*out_x += f->theme->tabsize - *out_x % f->theme->tabsize;
		
		if (f->buf->conts[i] == '\n' || *out_x > f->size_x - 1) {
			*out_x = GUTTER + linum_width;
			++*out_y;
		}
	}
}

void
frame_move_cursor(struct frame *f, unsigned x, unsigned y)
{
	size_t *csr = &f->cursor;
	*csr = 0;
	
	for (; *csr < f->buf->size && y > 0; ++*csr) {
		if (f->buf->conts[*csr] == '\n')
			--y;
	}

	for (; *csr < f->buf->size && f->buf->conts[*csr] != '\n' && x > 0; --x)
		++*csr;
}
