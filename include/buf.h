#ifndef BUF_H__
#define BUF_H__

#include <stdbool.h>
#include <stddef.h>

enum buf_src_type {
	BUF_SRC_TYPE_FRESH,
	BUF_SRC_TYPE_FILE,
};

struct buf {
	char *conts;
	size_t size, cap;
	
	enum buf_src_type src_type;
	void *src;
	
	bool writable;
	bool modified;
};

struct buf buf_create(bool writable);
struct buf buf_from_str(char const *str, bool writable);
struct buf buf_from_file(char const *path, bool writable);
int buf_save(struct buf *b);
void buf_destroy(struct buf *b);
void buf_write_ch(struct buf *b, size_t ind, char ch);
void buf_write_str(struct buf *b, size_t ind, char const *s);
void buf_erase(struct buf *b, size_t lb, size_t ub);
void buf_pos(struct buf const *b, size_t pos, unsigned *out_x, unsigned *out_y);

#endif
