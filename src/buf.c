#include "buf.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "prompt.h"

// adding entries past `MAXHISTSIZE` will cause the earliest existing entries to
// be deleted in order to make space.
#define MAXHISTSIZE 512

VEC_DEFIMPL(bufop)
VEC_DEFIMPL(pbuf)

extern bool flag_r;

static void pushhist(struct buf *b, enum bufoptype type, wchar_t const *data, size_t lb, size_t ub);

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
		.hist = vec_bufop_create(),
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
	b.flags = BF_WRITABLE | BF_NOHIST;
	b.srctype = BST_FILE;
	b.src = strdup(path);
	
	errno = 0;
	wint_t wch;
	while ((wch = fgetwc(fp)) != WEOF)
		buf_writewch(&b, b.size, wch);
	
	if (errno == EILSEQ) {
		size_t mlen = sizeof(wchar_t) * (strlen(path) + 31);
		wchar_t *msg = malloc(mlen);
		swprintf(msg, mlen, L"file contains invalid UTF-8: %s!", path);
		prompt_show(msg);
		free(msg);
	}

	fclose(fp);
	
	int ea = euidaccess(path, W_OK);
	if (ea != 0 || flag_r) {
		size_t mlen = sizeof(wchar_t) * (strlen(path) + 24);
		wchar_t *msg = malloc(mlen);
		swprintf(msg, mlen, L"opening file readonly: %s", path);
		prompt_show(msg);
		free(msg);
	}
	
	b.flags = (!ea && !flag_r) * BF_WRITABLE;
	
	return b;
}

struct buf
buf_fromwstr(wchar_t const *wstr, bool writable)
{
	struct buf b = buf_create(true);

	b.flags = BF_WRITABLE | BF_NOHIST;
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

int
buf_undo(struct buf *b)
{
	if (!(b->flags & BF_WRITABLE))
		return 1;

	while (b->hist.size > 0 && b->hist.data[b->hist.size - 1].type == BOT_BRK)
		vec_bufop_rm(&b->hist, b->hist.size - 1);

	// this is not a failure, so a 0 is returned.
	// having nothing happen upon undoing a non-action is the expected
	// result.
	if (b->hist.size == 0)
		return 0;

	struct bufop const *bo = &b->hist.data[b->hist.size - 1];
	switch (bo->type) {
	case BOT_WRITE:
		b->flags |= BF_NOHIST;
		buf_erase(b, bo->lb, bo->ub);
		b->flags &= ~BF_NOHIST;
		break;
	case BOT_ERASE:
		b->flags |= BF_NOHIST;
		buf_writewstr(b, bo->lb, bo->data);
		b->flags &= ~BF_NOHIST;
		free(bo->data);
		break;
	}

	vec_bufop_rm(&b->hist, b->hist.size - 1);
	buf_pushhistbrk(b);
	return 0;
}

void
buf_destroy(struct buf *b)
{
	free(b->conts);
	
	if (b->src)
		free(b->src);

	for (size_t i = 0; i < b->hist.size; ++i) {
		if (b->hist.data[i].data)
			free(b->hist.data[i].data);
	}
	vec_bufop_destroy(&b->hist);
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
	pushhist(b, BOT_WRITE, NULL, ind, ind + 1);
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
	pushhist(b, BOT_WRITE, NULL, ind, ind + len);
}

void
buf_erase(struct buf *b, size_t lb, size_t ub)
{
	if (!(b->flags & BF_WRITABLE))
		return;

	pushhist(b, BOT_ERASE, b->conts + lb, lb, ub);
	memmove(b->conts + lb, b->conts + ub, sizeof(wchar_t) * (b->size - ub));
	b->size -= ub - lb;
	b->flags |= BF_MODIFIED;
}

void
buf_pushhistbrk(struct buf *b)
{
	if (b->hist.size > 0)
		pushhist(b, BOT_BRK, NULL, 0, 1);
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

static void
pushhist(struct buf *b, enum bufoptype type, wchar_t const *data, size_t lb,
         size_t ub)
{
	if (b->flags & BF_NOHIST)
		return;

	if (lb >= ub)
		return;
	
	while (b->hist.size >= MAXHISTSIZE) {
		if (b->hist.data[0].data)
			free(b->hist.data[0].data);
		vec_bufop_rm(&b->hist, 0);
	}

	struct bufop *prev = b->hist.size > 0 ? &b->hist.data[b->hist.size - 1] : NULL;

	switch (type) {
	case BOT_WRITE:
		if (prev && prev->type == BOT_WRITE && lb == prev->ub)
			prev->ub = ub;
		else {
			struct bufop new = {
				.type = BOT_WRITE,
				.data = NULL,
				.lb = lb,
				.ub = ub,
			};
			vec_bufop_add(&b->hist, &new);
		}
		break;
	case BOT_ERASE:
		if (prev && prev->type == BOT_ERASE && ub == prev->lb) {
			size_t size = sizeof(wchar_t) * (ub - lb);
			size_t psize = sizeof(wchar_t) * (prev->ub - prev->lb);
			
			prev->data = realloc(prev->data, size + psize + sizeof(wchar_t));
			memmove(prev->data + ub - lb, prev->data, psize);
			memcpy(prev->data, data, size);
			prev->data[prev->ub - lb] = 0;
			prev->lb = lb;
		} else {
			struct bufop new = {
				.type = BOT_ERASE,
				.data = malloc(sizeof(wchar_t) * (ub - lb + 1)),
				.lb = lb,
				.ub = ub,
			};
			memcpy(new.data, data, sizeof(wchar_t) * (ub - lb));
			new.data[ub - lb] = 0;
			vec_bufop_add(&b->hist, &new);
		}
		break;
	case BOT_BRK: {
		struct bufop new = {
			.type = BOT_BRK,
			.data = NULL,
			.lb = 0,
			.ub = 0,
		};
		vec_bufop_add(&b->hist, &new);
		break;
	}
	}
}
