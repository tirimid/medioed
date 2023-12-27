#include "keybd.h"

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
static int curbind[KEYBD_MAXBINDLEN], curmac[KEYBD_MAXMACLEN];
static size_t curbindlen = 0, curmaclen = 0, curmacexec = 0;
static bool recmac = false, execmac = false;

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

void
keybd_recmac_begin(void)
{
	if (!execmac) {
		recmac = true;
		curmaclen = 0;
	}
}

bool
keybd_isrecmac(void)
{
	return recmac;
}

void
keybd_recmac_end(void)
{
	if (!execmac)
		recmac = false;
}

void
keybd_execmac(void)
{
	if (!recmac && curmaclen) {
		curmacexec = 0;
		execmac = true;
	}
}

bool
keybd_isexecmac(void)
{
	return execmac;
}

wint_t
keybd_awaitkey_nb(void)
{
	wint_t k;
	if (execmac) {
		// ignore the last key of the macro, as it is supposedly the one
		// that invoked the macro execution, so not ignoring it causes
		// infinite macro invocation.
		// `curbindlen` is reset for safety, in case the bind buffer
		// still contains part of the macro invocation bind.
		if (curmacexec < curmaclen - 1)
			k = curmac[curmacexec++];
		else {
			k = getwchar();
			execmac = false;
			curbindlen = 0;
		}
	} else
		k = getwchar();
	
	if (recmac) {
		if (curmaclen < KEYBD_MAXMACLEN)
			curmac[curmaclen++] = k;
	}
	
	return k;
}

wint_t
keybd_awaitkey(void)
{
	wint_t k = keybd_awaitkey_nb();
	
	if (curbindlen < KEYBD_MAXBINDLEN)
		curbind[curbindlen++] = k;
	else
		curbindlen = 0;

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

void
keybd_keydpy(wchar_t *out, int const *kbuf, size_t nk)
{
	*out = 0;
	
	for (size_t i = 0; i < nk; ++i) {
		wchar_t const *speckname;
		switch (kbuf[i]) {
		case K_BACKSPC:
			speckname = L"BACKSPC";
			break;
		case K_ESC:
			speckname = L"ESC";
			break;
		case K_RET:
			speckname = L"RET";
			break;
		case K_SPC:
			speckname = L"SPC";
			break;
		case K_TAB:
			speckname = L"TAB";
			break;
		default:
			speckname = NULL;
			break;
		}
		
		if (speckname)
			wcscat(out, speckname);
		else if (kbuf[i] <= 26) {
			wchar_t new[] = {L'C', L'-', L'a' + kbuf[i] - 1, 0};
			wcscat(out, new);
		} else {
			wchar_t new[] = {kbuf[i], 0};
			wcscat(out, new);
		}
		
		if (i < nk - 1)
			wcscat(out, L" ");
	}
}

int const *
keybd_curbind(size_t *out_len)
{
	if (out_len)
		*out_len = curbindlen;
	
	return curbindlen ? curbind : NULL;
}

int const *
keybd_curmac(size_t *out_len)
{
	if (out_len)
		*out_len = curmaclen;
	
	return curmaclen ? curmac : NULL;
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
