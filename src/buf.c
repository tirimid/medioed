#include "buf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "prompt.h"

VEC_DEFIMPL(pbuf)

struct buf
buf_create(bool writable)
{
	return (struct buf){
		.conts = malloc(sizeof(wchar_t)),
		.size = 0,
		.cap = 1,
		.src = NULL,
		.srctype = BST_FRESH,
		.flags = writable * BF_WRITABLE,
	};
}

struct buf
buf_fromfile(char const *path)
{
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		size_t mlen = sizeof(wchar_t) * (strlen(path) + 20);
		wchar_t *msg = malloc(mlen);
		swprintf(msg, mlen, L"cannot read file: %s!", path);
		prompt_show(msg);
		free(msg);
		return buf_create(false);
	}

	struct buf b = buf_create(true);
	b.srctype = BST_FILE;
	b.src = strdup(path);

	wint_t wch;
	while ((wch = fgetwc(fp)) != WEOF)
		buf_writewch(&b, b.size, wch);

	fclose(fp);
	
	int ea = euidaccess(path, W_OK);
	if (ea != 0) {
		size_t mlen = sizeof(wchar_t) * (strlen(path) + 24);
		wchar_t *msg = malloc(mlen);
		swprintf(msg, mlen, L"opening file readonly: %s", path);
		prompt_show(msg);
		free(msg);
	}
	
	b.flags = !ea * BF_WRITABLE;
	
	return b;
}

struct buf
buf_fromwstr(wchar_t const *wstr, bool writable)
{
	struct buf b = buf_create(true);
	
	buf_writewstr(&b, 0, wstr);
	b.flags = writable * BF_WRITABLE;

	return b;
}

int
buf_save(struct buf *b)
{
	// it doesnt make sense to save to anything other than a file and since
	// no file is specified as the buffer source, nothing is done.
	if (b->srctype != BST_FILE)
		return 1;

	// no point saving an unchanged file into itself.
	if (!(b->flags & BF_MODIFIED))
		return 0;

	FILE *fp = fopen(b->src, "wb");
	if (!fp)
		return 1;

	for (size_t i = 0; i < b->size; ++i) {
		wchar_t wcs[] = {b->conts[i], 0};
		char mbs[sizeof(wchar_t) + 1] = {0};
		wcstombs(mbs, wcs, sizeof(wchar_t) + 1);
		fputs(mbs, fp);
	}

	fclose(fp);
	b->flags &= ~BF_MODIFIED;
	
	return 0;
}

void
buf_destroy(struct buf *b)
{
	free(b->conts);
	if (b->src)
		free(b->src);
}

void
buf_writewch(struct buf *b, size_t ind, wchar_t wch)
{
	if (!(b->flags & BF_WRITABLE))
		return;

	if (b->size >= b->cap) {
		b->cap *= 2;
		b->conts = realloc(b->conts, sizeof(wchar_t) * b->cap);
	}

	memmove(b->conts + ind + 1, b->conts + ind, sizeof(wchar_t) * (b->size - ind));
	b->conts[ind] = wch;
	++b->size;
	b->flags |= BF_MODIFIED;
}

void
buf_writewstr(struct buf *b, size_t ind, wchar_t const *wstr)
{
	if (!(b->flags & BF_WRITABLE))
		return;

	size_t len = wcslen(wstr);
	size_t newcap = b->cap;
	
	for (size_t i = 1; i <= len; ++i) {
		if (b->size + i > newcap)
			newcap *= 2;
	}

	if (b->cap != newcap) {
		b->cap = newcap;
		b->conts = realloc(b->conts, sizeof(wchar_t) * b->cap);
	}

	memmove(b->conts + ind + len, b->conts + ind, sizeof(wchar_t) * (b->size - ind));
	memcpy(b->conts + ind, wstr, sizeof(wchar_t) * len);
	b->size += len;
	b->flags |= BF_MODIFIED;
}

void
buf_erase(struct buf *b, size_t lb, size_t ub)
{
	if (!(b->flags & BF_WRITABLE))
		return;

	memmove(b->conts + lb, b->conts + ub, sizeof(wchar_t) * (b->size - ub));
	b->size -= ub - lb;
	b->flags |= BF_MODIFIED;
}

void
buf_pos(struct buf const *b, size_t pos, unsigned *out_r, unsigned *out_c)
{
	*out_r = *out_c = 0;

	for (size_t i = 0; i < pos && i < b->size; ++i) {
		++*out_c;
		if (b->conts[i] == L'\n') {
			*out_c = 0;
			++*out_r;
		}
	}
}
