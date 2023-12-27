#include "prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>

#include "conf.h"
#include "draw.h"
#include "keybd.h"
#include "util.h"

#define BIND_CANCEL K_CTL('g')
#define BIND_NAVFWD K_CTL('f')
#define BIND_NAVBACK K_CTL('b')
#define BIND_DEL K_BACKSPC
#define BIND_COMPLETE K_TAB

static void drawbox(wchar_t const *text);

void
prompt_show(wchar_t const *msg)
{
	drawbox(msg);
	draw_refresh();
	
	for (;;) {
		wint_t k = keybd_awaitkey_nb();
		if (k == WEOF || k == L'q' || k == L'Q')
			break;
	}
}

wchar_t *
prompt_ask(wchar_t const *msg, void (*comp)(wchar_t **, size_t *, void *), void *cdata)
{
	drawbox(msg);

	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	// determine where the response should be rendered.
	unsigned rrow = ws.ws_row - 1, rcol = 0;
	for (wchar_t const *c = msg; *c; ++c) {
		if (*c == L'\n' || ++rcol > ws.ws_col)
			rcol = 0;
	}

	// a faux cursor is drawn before entering the keyboard loop, so that it
	// doesn't look like it spontaneously appears upon a keypress.
	draw_putattr(rrow, rcol, CONF_A_GHIGH, 1);
	draw_refresh();

	wchar_t *resp = malloc(sizeof(wchar_t));
	size_t resplen = 0;
	size_t csr = 0, dstart = 0;

	wint_t k;
	while ((k = keybd_awaitkey_nb()) != K_RET) {
		// gather response.
		if (k == BIND_CANCEL || k == WEOF) {
			free(resp);
			return NULL;
		} else if (k == BIND_NAVFWD)
			csr += csr < resplen;
		else if (k == BIND_NAVBACK)
			csr -= csr > 0;
		else if (k == BIND_DEL) {
			if (csr > 0) {
				memmove(resp + csr - 1, resp + csr, sizeof(wchar_t) * (resplen - csr));
				--resplen;
				--csr;
			}
		} else if (k == BIND_COMPLETE) {
			if (comp)
				comp(&resp, &resplen, cdata);
		} else {
			resp = realloc(resp, sizeof(wchar_t) * (++resplen + 1));
			memmove(resp + csr + 1, resp + csr, sizeof(wchar_t) * (resplen - csr));
			resp[csr++] = k;
		}

		// interactively render response.
		if (csr < dstart)
			dstart = csr;
		else if (csr - dstart >= ws.ws_col - rcol - 1)
			dstart = csr - ws.ws_col + rcol + 1;

		draw_fill(ws.ws_row - 1, rcol, 1, ws.ws_col - rcol, L' ', CONF_A_GNORM);
		for (size_t i = 0; i < resplen - dstart && i < ws.ws_col - rcol; ++i)
			draw_putwch(rrow, rcol + i, resp[dstart + i]);
		draw_putattr(rrow, rcol + csr - dstart, CONF_A_GHIGH, 1);

		draw_refresh();
	}

	resp[resplen] = 0;
	return resp;
}

int
prompt_yesno(wchar_t const *msg, bool deflt)
{
	size_t fullmsglen = wcslen(msg) + 8;
	wchar_t *fullmsg = malloc(sizeof(wchar_t) * fullmsglen);
	wchar_t const *defltmk = deflt ? L"(Y/n)" : L"(y/N)";
	swprintf(fullmsg, fullmsglen, L"%ls %ls ", msg, defltmk);
	
askagain:;
	wchar_t *resp = prompt_ask(fullmsg, NULL, NULL);
	if (!resp) {
		free(fullmsg);
		return -1;
	}
	
	if (!wcscmp(resp, L"y") || deflt && !*resp) {
		free(resp);
		free(fullmsg);
		return 1;
	} else if (!wcscmp(resp, L"n") || !deflt && !*resp) {
		free(resp);
		free(fullmsg);
		return 0;
	} else {
		free(resp);
		prompt_show(L"expected either 'y' or 'n'!");
		goto askagain;
	}
}

void
prompt_comp_path(wchar_t **resp, size_t *rlen, void *cdata)
{
}

static void
drawbox(wchar_t const *text)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	size_t textrows = 1, linewidth = 0;
	for (wchar_t const *c = text; *c; ++c) {
		if (++linewidth >= ws.ws_col || *c == L'\n') {
			linewidth = 0;
			++textrows;
		}
	}

	size_t boxtop = ws.ws_row - textrows;
	draw_fill(boxtop, 0, textrows, ws.ws_col, L' ', CONF_A_GNORM);
	draw_putwstr(boxtop, 0, text);
}
