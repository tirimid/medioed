#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

#include "buf.h"
#include "util.h"

typedef struct frame frame;

enum frame_draw_flag {
	FDF_ACTIVE = 0x1,
	FDF_MONO = 0x2,
};

struct frame {
	wchar_t *name;
	unsigned pr, pc;
	unsigned sr, sc;
	struct buf *buf;
	char *local_mode;
	size_t buf_start, csr;
	unsigned linum_width;
	unsigned csr_want_col;
};

VEC_DEF_PROTO(frame)

struct frame frame_create(wchar_t const *name, struct buf *buf);
void frame_destroy(struct frame *f);
void frame_draw(struct frame const *f, unsigned long flags);
void frame_pos(struct frame const *f, size_t pos, unsigned *out_r, unsigned *out_c);
void frame_mv_csr(struct frame *f, unsigned r, unsigned c);
void frame_mv_csr_rel(struct frame *f, int dr, int dc, bool wrap);
void frame_comp_boundary(struct frame *f);

#endif
