#include "keybd.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "conf.h"
#include "util.h"

typedef struct bind bind;

struct bind {
	int const *keyseq;
	size_t len;
	void (*fn)(void);
};

VEC_DEFPROTO_STATIC(bind)

static int compbinds(void const *a, void const *b);

static struct vec_bind binds;
static int curbind[KEYBD_MAXBINDLEN];
static size_t curbindlen = 0;

void
keybd_init(void)
{
	binds = vec_bind_create();
}

void
keybd_quit(void)
{
	curbindlen = 0;
	vec_bind_destroy(&binds);
}

void
keybd_bind(int const *keyseq, void (*fn)(void))
{
	size_t len = 0;
	while (keyseq[len] != -1)
		++len;
	
	for (size_t i = 0; i < binds.size; ++i) {
		if (binds.data[i].len == len
		    && !memcmp(keyseq, binds.data[i].keyseq, sizeof(int) * len)) {
			binds.data[i].fn = fn;
			return;
		}
	}

	struct bind new = {
		.keyseq = keyseq,
		.len = len,
		.fn = fn,
	};

	vec_bind_add(&binds, &new);
}

void
keybd_organize(void)
{
	qsort(binds.data, binds.size, sizeof(struct bind), compbinds);
}

wint_t
keybd_awaitkey(void)
{
	wint_t k = getwchar();
	
	curbind[curbindlen++] = k;

	ssize_t low = 0, high = binds.size - 1, mid;
	while (low <= high) {
		mid = (low + high) / 2;

		struct bind curbind_b = {
			.keyseq = curbind,
			.len = curbindlen,
		};
		
		int cmp = compbinds(&curbind_b, &binds.data[mid]);
		switch (cmp) {
		case -1:
			high = mid - 1;
			break;
		case 0:
			goto found;
		case 1:
			low = mid + 1;
			break;
		}
	}

	mid = -1;
found:
	if (mid != -1) {
		struct bind const *b = &binds.data[mid];
		if (b->len == curbindlen) {
			b->fn();
			curbindlen = 0;
		}

		return KEYBD_IGNORE;
	}
	
	curbindlen = 0;
	return k;
}

VEC_DEFIMPL_STATIC(bind)

static int
compbinds(void const *a, void const *b)
{
	struct bind const *binda = a, *bindb = b;

	for (size_t i = 0; i < binda->len && i < bindb->len; ++i) {
		if (binda->keyseq[i] > bindb->keyseq[i])
			return 1;
		else if (binda->keyseq[i] < bindb->keyseq[i])
			return -1;
	}

	return 0;
}
