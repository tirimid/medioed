#include "keybd.h"

#include <stdio.h>

#include "conf.h"
#include "util.h"

#define MAXEXBUFLEN 8

typedef struct bind bind;

struct bind {
	int keyseq[KEYBD_MAXBINDLEN];
	size_t len;
	void (*fn)(void);
};

VEC_DEFPROTO_STATIC(bind)

static struct vec_bind binds;
static int curbind[KEYBD_MAXBINDLEN];
static size_t curbindlen = 0;
static int exbuf[MAXEXBUFLEN];
static size_t exbuflen = 0;

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
		.len = len,
		.fn = fn,
	};
	memcpy(&new.keyseq, keyseq, sizeof(int) * len);

	vec_bind_add(&binds, &new);
}

wint_t
keybd_awaitkey(void)
{
	wint_t k = getwchar();
	if (k == WEOF) {
		if (exbuflen == 0)
			return KEYBD_IGNORE;

		k = exbuf[0];
		memmove(exbuf, exbuf + 1, --exbuflen * sizeof(int));
	}

	curbind[curbindlen++] = k;

	for (size_t i = 0; i < binds.size; ++i) {
		struct bind const *b = &binds.data[i];
		
		if (b->len == curbindlen
		    && !memcmp(curbind, b->keyseq, sizeof(int) * b->len)) {
			b->fn();
			curbindlen = 0;
			return KEYBD_IGNORE;
		}

		if (!memcmp(curbind, b->keyseq, sizeof(int) * curbindlen))
			return KEYBD_IGNORE;
	}

	curbindlen = 0;
	return k;
}

VEC_DEFIMPL_STATIC(bind)
