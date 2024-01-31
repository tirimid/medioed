#include "keybd.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "conf.h"
#include "util.h"

typedef struct bind bind;

struct bind {
	int const *key_seq;
	size_t len;
	void (*fn)(void);
};

VEC_DEF_PROTO_STATIC(bind)

static int cmp_binds(void const *a, void const *b);

static struct vec_bind binds;
static int cur_bind[KEYBD_MAX_BIND_LEN], cur_mac[KEYBD_MAX_MAC_LEN];
static size_t cur_bind_len = 0, cur_mac_len = 0, cur_mac_exec = 0;
static bool rec_mac = false, exec_mac = false;

void
keybd_init(void)
{
	binds = vec_bind_create();
}

void
keybd_quit(void)
{
	cur_bind_len = 0;
	vec_bind_destroy(&binds);
}

void
keybd_bind(int const *key_seq, void (*fn)(void))
{
	size_t len = 0;
	while (key_seq[len] != -1)
		++len;
	
	for (size_t i = 0; i < binds.size; ++i) {
		if (binds.data[i].len == len
		    && !memcmp(key_seq, binds.data[i].key_seq, sizeof(int) * len)) {
			binds.data[i].fn = fn;
			return;
		}
	}

	struct bind new = {
		.key_seq = key_seq,
		.len = len,
		.fn = fn,
	};

	vec_bind_add(&binds, &new);
}

void
keybd_organize(void)
{
	qsort(binds.data, binds.size, sizeof(struct bind), cmp_binds);
}

void
keybd_rec_mac_begin(void)
{
	if (!exec_mac) {
		rec_mac = true;
		cur_mac_len = 0;
	}
}

bool
keybd_is_rec_mac(void)
{
	return rec_mac;
}

void
keybd_rec_mac_end(void)
{
	if (!exec_mac)
		rec_mac = false;
}

void
keybd_exec_mac(void)
{
	if (!rec_mac && cur_mac_len) {
		cur_mac_exec = 0;
		exec_mac = true;
	}
}

bool
keybd_is_exec_mac(void)
{
	return exec_mac;
}

wint_t
keybd_await_key_nb(void)
{
	wint_t k;
	if (exec_mac) {
		// ignore the last key of the macro, as it is supposedly the one
		// that invoked the macro execution, so not ignoring it causes
		// infinite macro invocation.
		// `cur_bind_len` is reset for safety, in case the bind buffer
		// still contains part of the macro invocation bind.
		if (cur_mac_exec < cur_mac_len - 1)
			k = cur_mac[cur_mac_exec++];
		else {
			k = getwchar();
			exec_mac = false;
			cur_bind_len = 0;
		}
	} else
		k = getwchar();
	
	if (rec_mac) {
		if (cur_mac_len < KEYBD_MAX_MAC_LEN)
			cur_mac[cur_mac_len++] = k;
	}
	
	return k;
}

wint_t
keybd_await_key(void)
{
	wint_t k = keybd_await_key_nb();
	
	if (cur_bind_len < KEYBD_MAX_BIND_LEN)
		cur_bind[cur_bind_len++] = k;
	else
		cur_bind_len = 0;

	ssize_t low = 0, high = binds.size - 1, mid;
	while (low <= high) {
		mid = (low + high) / 2;

		struct bind cur_bind_b = {
			.key_seq = cur_bind,
			.len = cur_bind_len,
		};
		
		int cmp = cmp_binds(&cur_bind_b, &binds.data[mid]);
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
		
		if (b->len == cur_bind_len) {
			b->fn();
			cur_bind_len = 0;
		}

		return KEYBD_IGNORE;
	}
	
	cur_bind_len = 0;
	return k;
}

void
keybd_key_dpy(wchar_t *out, int const *kbuf, size_t nk)
{
	*out = 0;
	
	for (size_t i = 0; i < nk; ++i) {
		wchar_t const *spec_k_name;
		switch (kbuf[i]) {
		case K_BACKSPC:
			spec_k_name = L"BACKSPC";
			break;
		case K_ESC:
			spec_k_name = L"ESC";
			break;
		case K_RET:
			spec_k_name = L"RET";
			break;
		case K_SPC:
			spec_k_name = L"SPC";
			break;
		case K_TAB:
			spec_k_name = L"TAB";
			break;
		default:
			spec_k_name = NULL;
			break;
		}
		
		if (spec_k_name)
			wcscat(out, spec_k_name);
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
keybd_cur_bind(size_t *out_len)
{
	if (out_len)
		*out_len = cur_bind_len;
	
	return cur_bind_len ? cur_bind : NULL;
}

int const *
keybd_cur_mac(size_t *out_len)
{
	if (out_len)
		*out_len = cur_mac_len;
	
	return cur_mac_len ? cur_mac : NULL;
}

VEC_DEF_IMPL_STATIC(bind)

static int
cmp_binds(void const *a, void const *b)
{
	struct bind const *bind_a = a, *bind_b = b;

	for (size_t i = 0; i < bind_a->len && i < bind_b->len; ++i) {
		if (bind_a->key_seq[i] > bind_b->key_seq[i])
			return 1;
		else if (bind_a->key_seq[i] < bind_b->key_seq[i])
			return -1;
	}

	return 0;
}
