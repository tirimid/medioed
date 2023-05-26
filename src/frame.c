#include "frame.h"

#include <stdlib.h>

#include <ncurses.h>

struct frame_theme
frame_theme_default(void)
{
	return (struct frame_theme){
		.norm_fg = COLOR_WHITE,
		.norm_bg = COLOR_BLACK,
		.cursor_fg = COLOR_BLACK,
		.cursor_bg = COLOR_WHITE,
		.highlights = arraylist_create(),
	};
}

struct frame_theme
frame_theme_load(char const *theme_path)
{
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

static void
set_attr_at(int attr, unsigned x, unsigned y)
{
	char ch = mvinch(y, x);
	attron(attr);
	mvaddch(y, x, ch);
}

void
frame_draw(struct frame const *f)
{
	// write characters.
	size_t drawcsr = f->buf_start;
	for (size_t i = 0; i < f->size_y; ++i) {
		for (size_t j = 0; j < f->size_x; ++j) {
			char ch = drawcsr >= f->buf->size ? ' ' : f->buf->conts[drawcsr++];
			mvaddch(f->pos_y + i, f->pos_x + j, ch);
		}
	}
	
	// set coloration, attributes, etc.
	init_pair(1, f->theme->norm_fg, f->theme->norm_bg);
	for (size_t i = 0; i < f->size_x; ++i) {
		for (size_t j = 0; j < f->size_y; ++j)
			set_attr_at(COLOR_PAIR(1), f->pos_x + i, f->pos_y + j);
	}

	unsigned csrx, csry;
	frame_cursor_pos(f, &csrx, &csry);
	init_pair(2, f->theme->cursor_fg, f->theme->cursor_bg);
	set_attr_at(COLOR_PAIR(2), f->pos_x + csrx, f->pos_y + csry);
}

void
frame_cursor_pos(struct frame const *f, unsigned *out_x, unsigned *out_y)
{
	*out_x = *out_y = 0;
	for (size_t i = f->buf_start; i < f->cursor; ++i) {
		++*out_x;
		if (f->buf->conts[i] == '\n') {
			++*out_y;
			*out_x = 0;
		}
	}
}
