#include "buf.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "prompt.h"

// adding entries past `MAX_HIST_SIZE` will cause the earliest existing entries
// to be deleted in order to make space.
#define MAX_HIST_SIZE 512

VEC_DEF_IMPL(struct buf_op, buf_op)
VEC_DEF_IMPL(struct buf *, p_buf)

extern bool flag_r;

static void push_hist(struct buf *b, enum buf_op_type type, wchar_t const *data, size_t lb, size_t ub);

// used for returning `buf_get_wstr()`.
// this is a horribly ugly hack that ought to eventually be removed.
// TODO: REMOVE THIS UTTER GARBAGE AS SOON AS POSSIBLE.
static wchar_t *wstr_sv = NULL;
static size_t wstr_sv_len = 0;

void
buf_sys_init(void)
{
	wstr_sv = malloc(1);
}

void
buf_sys_quit(void)
{
	free(wstr_sv);
}

struct buf
buf_create(bool writable)
{
	return (struct buf){
		.conts_ = malloc(sizeof(wchar_t)),
		.size = 0,
		.cap = 1,
		.src = NULL,
		.src_type = BST_FRESH,
		.flags = writable * BF_WRITABLE,
		.hist = vec_buf_op_create(),
	};
}

struct buf
buf_from_file(char const *path)
{
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		size_t msg_len = sizeof(wchar_t) * (strlen(path) + 20);
		wchar_t *msg = malloc(msg_len);
		swprintf(msg, msg_len, L"cannot read file: %s!", path);
		prompt_show(msg);
		free(msg);
		return buf_create(false);
	}

	struct buf b = buf_create(true);
	b.flags = BF_WRITABLE | BF_NO_HIST;
	b.src_type = BST_FILE;
	b.src = strdup(path);
	
	errno = 0;
	wint_t wch;
	while ((wch = fgetwc(fp)) != WEOF)
		buf_write_wch(&b, b.size, wch);
	
	if (errno == EILSEQ) {
		size_t msg_len = sizeof(wchar_t) * (strlen(path) + 31);
		wchar_t *msg = malloc(msg_len);
		swprintf(msg, msg_len, L"file contains invalid UTF-8: %s!", path);
		prompt_show(msg);
		free(msg);
	}

	fclose(fp);
	
	int ea = euidaccess(path, W_OK);
	if (ea != 0 || flag_r) {
		size_t msg_len = sizeof(wchar_t) * (strlen(path) + 24);
		wchar_t *msg = malloc(msg_len);
		swprintf(msg, msg_len, L"opening file readonly: %s", path);
		prompt_show(msg);
		free(msg);
	}
	
	b.flags = (!ea && !flag_r) * BF_WRITABLE;
	
	return b;
}

struct buf
buf_from_wstr(wchar_t const *wstr, bool writable)
{
	struct buf b = buf_create(true);

	b.flags = BF_WRITABLE | BF_NO_HIST;
	buf_write_wstr(&b, 0, wstr);
	b.flags = writable * BF_WRITABLE;

	return b;
}

int
buf_save(struct buf *b)
{
	// it doesnt make sense to save to anything other than a file and since
	// no file is specified as the buffer source, nothing is done.
	if (b->src_type != BST_FILE)
		return 1;

	// no point saving an unchanged file into itself.
	if (!(b->flags & BF_MODIFIED))
		return 0;
	
	FILE *fp = fopen(b->src, "wb");
	if (!fp)
		return 1;

	for (size_t i = 0; i < b->size; ++i) {
		wchar_t wcs[] = {b->conts_[i], 0};
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
		vec_buf_op_rm(&b->hist, b->hist.size - 1);

	// this is not a failure, so a 0 is returned.
	// having nothing happen upon undoing a non-action is the expected
	// result.
	if (b->hist.size == 0)
		return 0;

	struct buf_op const *bo = &b->hist.data[b->hist.size - 1];
	switch (bo->type) {
	case BOT_WRITE:
		b->flags |= BF_NO_HIST;
		buf_erase(b, bo->lb, bo->ub);
		b->flags &= ~BF_NO_HIST;
		break;
	case BOT_ERASE:
		b->flags |= BF_NO_HIST;
		buf_write_wstr(b, bo->lb, bo->data);
		b->flags &= ~BF_NO_HIST;
		free(bo->data);
		break;
	}

	vec_buf_op_rm(&b->hist, b->hist.size - 1);
	buf_push_hist_brk(b);
	return 0;
}

void
buf_destroy(struct buf *b)
{
	free(b->conts_);
	
	if (b->src)
		free(b->src);

	for (size_t i = 0; i < b->hist.size; ++i) {
		if (b->hist.data[i].data)
			free(b->hist.data[i].data);
	}
	vec_buf_op_destroy(&b->hist);
}

void
buf_write_wch(struct buf *b, size_t ind, wchar_t wch)
{
	if (!(b->flags & BF_WRITABLE))
		return;

	if (b->size >= b->cap) {
		b->cap *= 2;
		b->conts_ = realloc(b->conts_, sizeof(wchar_t) * b->cap);
	}

	memmove(b->conts_ + ind + 1, b->conts_ + ind, sizeof(wchar_t) * (b->size - ind));
	b->conts_[ind] = wch;
	++b->size;
	b->flags |= BF_MODIFIED;
	push_hist(b, BOT_WRITE, NULL, ind, ind + 1);
}

void
buf_write_wstr(struct buf *b, size_t ind, wchar_t const *wstr)
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
		b->conts_ = realloc(b->conts_, sizeof(wchar_t) * b->cap);
	}

	memmove(b->conts_ + ind + len, b->conts_ + ind, sizeof(wchar_t) * (b->size - ind));
	memcpy(b->conts_ + ind, wstr, sizeof(wchar_t) * len);
	b->size += len;
	b->flags |= BF_MODIFIED;
	push_hist(b, BOT_WRITE, NULL, ind, ind + len);
}

void
buf_erase(struct buf *b, size_t lb, size_t ub)
{
	if (!(b->flags & BF_WRITABLE))
		return;

	push_hist(b, BOT_ERASE, b->conts_ + lb, lb, ub);
	memmove(b->conts_ + lb, b->conts_ + ub, sizeof(wchar_t) * (b->size - ub));
	b->size -= ub - lb;
	b->flags |= BF_MODIFIED;
}

void
buf_push_hist_brk(struct buf *b)
{
	if (b->hist.size > 0)
		push_hist(b, BOT_BRK, NULL, 0, 1);
}

void
buf_pos(struct buf const *b, size_t pos, unsigned *out_r, unsigned *out_c)
{
	*out_r = *out_c = 0;

	for (size_t i = 0; i < pos && i < b->size; ++i) {
		++*out_c;
		if (b->conts_[i] == L'\n') {
			*out_c = 0;
			++*out_r;
		}
	}
}

wchar_t
buf_get_wch(struct buf const *b, size_t ind)
{
	return ind >= b->size ? 0 : b->conts_[ind];
}

wchar_t const *
buf_get_wstr(struct buf const *b, size_t ind, size_t n)
{
	if (n == 0 || ind + n > b->size)
		return NULL;
	
	if (n > wstr_sv_len) {
		wstr_sv = realloc(wstr_sv, sizeof(wchar_t) * (n + 1));
		wstr_sv_len = n;
	}
	
	wcsncpy(wstr_sv, &b->conts_[ind], n);
	wstr_sv[n] = 0;
	
	return wstr_sv;
}

static void
push_hist(struct buf *b, enum buf_op_type type, wchar_t const *data, size_t lb,
          size_t ub)
{
	if (b->flags & BF_NO_HIST)
		return;

	if (lb >= ub)
		return;
	
	while (b->hist.size >= MAX_HIST_SIZE) {
		if (b->hist.data[0].data)
			free(b->hist.data[0].data);
		vec_buf_op_rm(&b->hist, 0);
	}

	struct buf_op *prev = b->hist.size > 0 ? &b->hist.data[b->hist.size - 1] : NULL;

	switch (type) {
	case BOT_WRITE:
		if (prev && prev->type == BOT_WRITE && lb == prev->ub)
			prev->ub = ub;
		else {
			struct buf_op new = {
				.type = BOT_WRITE,
				.data = NULL,
				.lb = lb,
				.ub = ub,
			};
			vec_buf_op_add(&b->hist, &new);
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
			struct buf_op new = {
				.type = BOT_ERASE,
				.data = malloc(sizeof(wchar_t) * (ub - lb + 1)),
				.lb = lb,
				.ub = ub,
			};
			memcpy(new.data, data, sizeof(wchar_t) * (ub - lb));
			new.data[ub - lb] = 0;
			vec_buf_op_add(&b->hist, &new);
		}
		break;
	case BOT_BRK: {
		struct buf_op new = {
			.type = BOT_BRK,
			.data = NULL,
			.lb = 0,
			.ub = 0,
		};
		vec_buf_op_add(&b->hist, &new);
		break;
	}
	}
}
