#ifndef BUF_H
#define BUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "util.h"

enum buf_src_type {
	BST_FRESH = 0,
	BST_FILE,
};

enum buf_flag {
	BF_WRITABLE = 0x1,
	BF_MODIFIED = 0x2,
	BF_NO_HIST = 0x4,
};

enum buf_op_type {
	BOT_WRITE = 0,
	BOT_ERASE,
	BOT_BRK,
};

struct buf_op {
	wchar_t *data;
	size_t lb, ub;
	unsigned char type;
};

VEC_DEF_PROTO(struct buf_op, buf_op)

struct buf {
	// TODO: change name back and implement gap buffer system.
	// name suffixed with `_` to raise errors where it was previously used,
	// so that they can be changed.
	wchar_t *conts_;
	
	// `size` ignores the size of the internal gap.
	// e.g. a five-wchar gap with two wchars around it would yield a buffer
	// size of 2.
	size_t size;
	
	size_t cap;
	size_t gap_pos, gap_size;
	void *src;
	unsigned char src_type;
	uint8_t flags;
	struct vec_buf_op hist;
};

VEC_DEF_PROTO(struct buf *, p_buf)

void buf_sys_init(void);
void buf_sys_quit(void);

struct buf buf_create(bool writable);
struct buf buf_from_file(char const *path);
struct buf buf_from_wstr(wchar_t const *wstr, bool writable);
int buf_save(struct buf *b);
int buf_undo(struct buf *b);
void buf_destroy(struct buf *b);
void buf_write_wch(struct buf *b, size_t ind, wchar_t wch);
void buf_write_wstr(struct buf *b, size_t ind, wchar_t const *wstr);
void buf_erase(struct buf *b, size_t lb, size_t ub);
void buf_push_hist_brk(struct buf *b);
void buf_pos(struct buf const *b, size_t pos, unsigned *out_r, unsigned *out_c);
wchar_t buf_get_wch(struct buf const *b, size_t ind);

// this function is NOT thread-safe.
// the program is not multi-threaded, so this will cause no issue.
// calling this function will invalidate any previously returned values, so if
// you need to compare multiple regions against eachother, you must save the
// output of each call to its own string.
wchar_t const *buf_get_wstr(struct buf const *b, size_t ind, size_t n);

#endif
