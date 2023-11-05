#ifndef BUF_H__
#define BUF_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "util.h"

typedef struct buf *pbuf;
typedef struct bufop bufop;

enum bufsrctype {
	BST_FRESH = 0,
	BST_FILE,
};

enum bufflag {
	BF_WRITABLE = 0x1,
	BF_MODIFIED = 0x2,
	BF_NOHIST = 0x4,
};

enum bufoptype {
	BOT_WRITE = 0,
	BOT_ERASE,
	BOT_BRK,
};

struct bufop {
	wchar_t *data;
	size_t lb, ub;
	unsigned char type;
};

VEC_DEFPROTO(bufop)

struct buf {
	wchar_t *conts;
	size_t size, cap;
	void *src;
	unsigned char srctype;
	uint8_t flags;
	struct vec_bufop hist;
};

VEC_DEFPROTO(pbuf)

struct buf buf_create(bool writable);
struct buf buf_fromfile(char const *path);
struct buf buf_fromwstr(wchar_t const *wstr, bool writable);
int buf_save(struct buf *b);
int buf_undo(struct buf *b);
void buf_destroy(struct buf *b);
void buf_writewch(struct buf *b, size_t ind, wchar_t wch);
void buf_writewstr(struct buf *b, size_t ind, wchar_t const *wstr);
void buf_erase(struct buf *b, size_t lb, size_t ub);
void buf_pushhistbrk(struct buf *b);
void buf_pos(struct buf const *b, size_t pos, unsigned *out_r, unsigned *out_c);

#endif
