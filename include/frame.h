#ifndef FRAME_H__
#define FRAME_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

#include "buf.h"
#include "util.h"

struct frame {
	char *name;
	unsigned pos_x, pos_y;
	unsigned size_x, size_y;
	struct buf *buf;

	// use `frame_(rel)move_cursor()` instead of writing directly, otherwise
	// visual frame effects will be messed up (scrolling, wrapping, etc.).
	size_t buf_start, cursor;
	unsigned linumw;

	char *localmode;
};

struct frame frame_create(char const *name, struct buf *buf);
void frame_destroy(struct frame *f);
void frame_prepare_draw(struct frame *f);
void frame_draw(struct frame const *f, bool active);
void frame_pos(struct frame const *f, size_t pos, unsigned *out_x, unsigned *out_y);
void frame_move_cursor(struct frame *f, unsigned x, unsigned y);
void frame_relmove_cursor(struct frame *f, int x, int y, bool lwrap);

#endif
