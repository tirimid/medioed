#ifndef FRAME_H__
#define FRAME_H__

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

#include "buf.h"
#include "util.h"

typedef struct frame frame;

struct frame {
	wchar_t *name;
	unsigned pr, pc;
	unsigned sr, sc;
	struct buf *buf;
	char *localmode;
	size_t bufstart, csr;
	unsigned linumw;
	unsigned csr_wantcol;
};

VEC_DEFPROTO(frame)

struct frame frame_create(wchar_t const *name, struct buf *buf);
void frame_destroy(struct frame *f);
void frame_draw(struct frame const *f, bool active);
void frame_pos(struct frame const *f, size_t pos, unsigned *out_r, unsigned *out_c);
void frame_mvcsr(struct frame *f, unsigned r, unsigned c);
void frame_relmvcsr(struct frame *f, int dr, int dc, bool lwrap);
void frame_compbndry(struct frame *f);

#endif
