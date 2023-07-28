#ifndef FRAME_H__
#define FRAME_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "buf.h"
#include "util.h"

struct frame_theme_highlight {
	char *regex;
	uint8_t fg, bg;
	short pair;
};

// frames use non-global configuration.
struct frame_theme {
	// coloration options.
	// this only allows for indexed-color terminals with up to 256 colors.
	uint8_t norm_fg, norm_bg;
	short norm_pair;
	uint8_t cursor_fg, cursor_bg;
	short cursor_pair;
	uint8_t linum_fg, linum_bg;
	short linum_pair;
	
	struct arraylist highlights;

	// other text rendering options.
	unsigned tabsize;
};

struct frame {
	char *name;
	unsigned pos_x, pos_y;
	unsigned size_x, size_y;
	struct buf *buf;
	
	// use `frame_(rel)move_cursor()` instead of writing directly, otherwise
	// visual frame effects will be messed up (scrolling, wrapping, etc.).
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
void frame_pos(struct frame const *f, size_t pos, unsigned *out_x,
               unsigned *out_y);
void frame_move_cursor(struct frame *f, unsigned x, unsigned y);
void frame_relmove_cursor(struct frame *f, int x, int y, bool lwrap);

#endif
