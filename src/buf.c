#include "buf.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libtmcul/file.h>

#define SIZE_ADD 4096

struct buf
buf_create(void)
{
	return (struct buf){
		.conts = malloc(SIZE_ADD),
		.size = 0,
		.cap = SIZE_ADD,
	};
}

struct buf
buf_from_file(char const *path)
{
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return buf_create();
	
	size_t cap = file_size_direct(fp), size;
	cap += SIZE_ADD - cap % SIZE_ADD;
	char *conts = realloc(file_read_direct(fp, &size), cap);
	
	fclose(fp);

	return (struct buf){
		.conts = conts,
		.size = size,
		.cap = cap,
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
	if (b->size + 1 > b->cap) {
		b->cap += SIZE_ADD - b->cap % SIZE_ADD;
		b->conts = realloc(b->conts, b->cap);
	}

	memmove(b->conts + ind + 1, b->conts + ind, b->size - ind);
	b->conts[ind] = ch;
	++b->size;
}

void
buf_write_str(struct buf *b, size_t ind, char const *s)
{
	size_t len = strlen(s);
	size_t new_cap = b->cap;
	
	for (size_t i = 1; i <= len; ++i) {
		if (b->size + i > new_cap)
			new_cap += SIZE_ADD - new_cap % SIZE_ADD;
	}

	if (b->cap != new_cap) {
		b->cap = new_cap;
		b->conts = realloc(b->conts, b->cap);
	}

	memmove(b->conts + ind + len, b->conts + ind, b->size - ind);
	strcpy(b->conts + ind, s);
	b->size += len;
}

void
buf_erase(struct buf *b, size_t lb, size_t ub)
{
	memmove(b->conts + lb, b->conts + ub, b->size - ub);
	b->size -= ub - lb;
}
