#include "frame.h"

#include <stdlib.h>

#include <ncurses.h>

struct frame_theme
frame_theme_default(void)
{
	return (struct frame_theme){
		.norm_fg = COLOR_WHITE,
		.norm_bg = COLOR_BLUE,
		.cursor_fg = COLOR_BLACK,
		.cursor_bg = COLOR_WHITE,
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
	// clear frame.
	for (size_t i = 0; i < f->size_y; ++i) {
		for (size_t j = 0; j < f->size_x; ++j)
			mvaddch(f->pos_y + i, f->pos_x + j, ' ');
	}
	
	// write characters.
	size_t drawcsr = f->buf_start;
	for (size_t i = 0; i < f->size_y; ++i) {
		for (size_t j = 0; j < f->size_x; ++j) {
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
				mvaddch(f->pos_y + i, f->pos_x + j, ch);
				break;
			}
		}
	skip_x:;
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

	// TODO: set highlight coloration.
}

void
frame_cursor_pos(struct frame const *f, unsigned *out_x, unsigned *out_y)
{
	*out_x = *out_y = 0;
	for (size_t i = f->buf_start; i < f->cursor; ++i) {
		++*out_x;

		if (f->buf->conts[i] == '\t')
			*out_x += f->theme->tabsize - *out_x % f->theme->tabsize;
		
		if (f->buf->conts[i] == '\n' || *out_x > f->size_x - 1) {
			++*out_y;
			*out_x = 0;
		}
	}
}
