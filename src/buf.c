#include "buf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct buf
buf_create(bool writable)
{
	return (struct buf){
		.conts = malloc(1),
		.size = 0,
		.cap = 1,
		.writable = writable
	};
}

struct buf
buf_from_str(char const *str, bool writable)
{
	struct buf b = buf_create(true);
	
	buf_write_str(&b, 0, str);
	b.writable = writable;

	return b;
}

struct buf
buf_from_file(char const *path, bool writable)
{
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return buf_create(writable);

	fseek(fp, 0, SEEK_END);
	size_t fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	size_t cap;
	for (cap = 1; cap < fsize; cap *= 2);
	
	char *fconts = malloc(cap);
	fread(fconts, 1, fsize, fp);
	
	fclose(fp);

	return (struct buf){
		.conts = fconts,
		.size = fsize,
		.cap = cap,
		.writable = writable,
	};
}

void
buf_destroy(struct buf *b)
{
	free(b->conts);
}

void
buf_write_ch(struct buf *b, size_t ind, char ch)
{
	if (!b->writable)
		return;
	
	if (b->size >= b->cap) {
		b->cap *= 2;
		b->conts = realloc(b->conts, b->cap);
	}

	memmove(b->conts + ind + 1, b->conts + ind, b->size - ind);
	b->conts[ind] = ch;
	++b->size;
}

void
buf_write_str(struct buf *b, size_t ind, char const *s)
{
	if (!b->writable)
		return;
	
	size_t len = strlen(s);
	size_t new_cap = b->cap;
	
	for (size_t i = 1; i <= len; ++i) {
		if (b->size + i > new_cap)
			new_cap *= 2;
	}

	if (b->cap != new_cap) {
		b->cap = new_cap;
		b->conts = realloc(b->conts, b->cap);
	}

	memmove(b->conts + ind + len, b->conts + ind, b->size - ind);
	memcpy(b->conts + ind, s, len);
	b->size += len;
}

void
buf_erase(struct buf *b, size_t lb, size_t ub)
{
	if (!b->writable)
		return;
	
	memmove(b->conts + lb, b->conts + ub, b->size - ub);
	b->size -= ub - lb;
}

void
buf_pos(struct buf const *b, size_t pos, unsigned *out_x, unsigned *out_y)
{
	*out_x = *out_y = 0;
	
	for (size_t i = 0; i < pos; ++i) {
		++*out_x;
		if (b->conts[i] == '\n') {
			*out_x = 0;
			++*out_y;
		}
	}
}
