#ifndef FRAME_H__
#define FRAME_H__

#define _POSIX_C_SOURCE 200809
#include <string.h>
#undef _POSIX_C_SOURCE

#include <stdint.h>
#include <stddef.h>

#include <tmcul/ds/arraylist.h>

#include "buf.h"

struct frame_theme_highlight {
	char *regex;
	uint8_t fg, bg;
};

struct frame_theme {
	// coloration options.
	// this only allows for indexed-color terminals with up to 256 colors.
	uint8_t norm_fg, norm_bg;
	uint8_t cursor_fg, cursor_bg;
	uint8_t linum_fg, linum_bg;
	struct arraylist highlights;

	// other text rendering options.
	unsigned tabsize;
};

struct frame {
	char *name;
	unsigned pos_x, pos_y;
	unsigned size_x, size_y;
	struct buf *buf;
	size_t buf_start, cursor;
	struct frame_theme const *theme;
};

struct frame_theme frame_theme_default(void);
struct frame_theme frame_theme_load(char const *theme_path);
void frame_theme_destroy(struct frame_theme *ft);
struct frame frame_create(char const *name, unsigned px, unsigned py,
                          unsigned sx, unsigned sy, struct buf *buf,
                          struct frame_theme const *theme);
void frame_destroy(struct frame *f);
void frame_draw(struct frame const *f);
void frame_cursor_pos(struct frame const *f, unsigned *out_x, unsigned *out_y);

#endif
