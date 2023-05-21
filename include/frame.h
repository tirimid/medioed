#ifndef FRAME_H__
#define FRAME_H__

#include <stdint.h>
#include <stddef.h>

#include <libtmcul/ds/arraylist.h>

#include "buf.h"

struct frame_theme_highlight {
	char const *regex;
	uint8_t fg, bg;
};

// this only allows for indexed-color terminals with up to 256 colors.
struct frame_theme {
	uint8_t norm_fg, norm_bg;
	uint8_t cursor_fg, cursor_bg;
	struct arraylist highlights;
};

struct frame {
	char *name;
	unsigned pos_x, pos_y;
	unsigned size_x, size_y;
	struct buf *buf;
	size_t cursor;
	struct frame_theme theme;
};

struct frame_theme frame_theme_load(char const *theme_path);
void frame_theme_destroy(struct frame_theme *ft);
void frame_destroy(struct frame *f);
void frame_draw(struct frame const *f);

#endif
