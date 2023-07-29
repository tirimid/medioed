#ifndef BUF_H__
#define BUF_H__

#include <stdbool.h>
#include <stddef.h>

struct buf {
	char *conts;
	size_t size, cap;
	bool writable;
};

struct buf buf_create(bool writable);
struct buf buf_from_str(char const *str, bool writable);
struct buf buf_from_file(char const *path, bool writable);
void buf_destroy(struct buf *b);
void buf_write_ch(struct buf *b, size_t ind, char ch);
void buf_write_str(struct buf *b, size_t ind, char const *s);
void buf_erase(struct buf *b, size_t lb, size_t ub);
void buf_pos(struct buf const *b, size_t pos, unsigned *out_x, unsigned *out_y);

#endif
