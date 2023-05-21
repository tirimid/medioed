#ifndef BUF_H__
#define BUF_H__

#include <stddef.h>

struct buf {
	char *conts;
	size_t size, cap;
};

struct buf buf_create(void);
struct buf buf_from_file(char const *path);
void buf_destroy(struct buf *b);
void buf_write_ch(struct buf *b, size_t ind, char ch);
void buf_write_str(struct buf *b, size_t ind, char const *s);
void buf_erase(struct buf *b, size_t lb, size_t ub);

#endif
