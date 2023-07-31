#include "prompt.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>
#include <sys/ioctl.h>

#include "def.h"

#define CANCEL_BIND "^G"
#define NAVFWD_BIND "^F"
#define NAVBACK_BIND "^B"
#define DEL_BIND "^?"
#define COMPLETE_BIND "^I"

static void drawbox(char const *text);

void
prompt_show(char const *text)
{
	drawbox(text);
	
	int k;
	while ((k = getch()) != 'q' && k != 'Q');
}

char *
prompt_ask(char const *text, void (*complete)(char **, size_t *, void *),
           void *compdata)
{
	drawbox(text);

	struct winsize tty_size;
	ioctl(0, TIOCGWINSZ, &tty_size);
	unsigned ttyx = tty_size.ws_col, ttyy = tty_size.ws_row;

	// determine where the response should be rendered.
	unsigned rrow = ttyy - 1, rcol = 0;
	for (char const *c = text; *c; ++c) {
		if (*c == '\n' || ++rcol > ttyx)
			rcol = 0;
	}

	// a faux cursor is drawn before entering the keyboard loop, so that it
	// doesn't look like it spontaneously appears upon a keypress.
	mvchgat(rrow, rcol, 1, 0, GLOBAL_HIGHLIGHT_PAIR, NULL);
	refresh();

	char *resp = malloc(1);
	size_t resp_len = 0;
	size_t csr = 0, dstart = 0;

	int k;
	while ((k = getch()) != '\n') {
		// gather response.
		char const *kname = keyname(k);
		
		if (!strcmp(kname, CANCEL_BIND)) {
			free(resp);
			return NULL;
		} else if (!strcmp(kname, NAVFWD_BIND))
			csr += csr < resp_len;
		else if (!strcmp(kname, NAVBACK_BIND))
			csr -= csr > 0;
		else if (!strcmp(kname, DEL_BIND)) {
			if (csr > 0) {
				memmove(resp + csr - 1, resp + csr, resp_len - csr);
				--resp_len;
				--csr;
			}
		} else if (!strcmp(kname, COMPLETE_BIND)) {
			if (complete)
				complete(&resp, &resp_len, compdata);
		} else {
			resp = realloc(resp, ++resp_len + 1);
			memmove(resp + csr + 1, resp + csr, resp_len - csr);
			resp[csr] = (char)k;
			++csr;
		}

		// interactively render response.
		if (csr < dstart)
			dstart = csr;
		else if (csr - dstart >= ttyx - rcol - 1)
			dstart = csr - ttyx + rcol + 1;

		for (unsigned i = rcol; i < ttyx; ++i)
			mvaddch(rrow, i, ' ');
		
		for (size_t i = 0; i < resp_len - dstart && i < ttyx - rcol; ++i)
			mvaddch(rrow, rcol + i, resp[dstart + i]);

		mvchgat(rrow, rcol, ttyx - rcol, 0, GLOBAL_NORM_PAIR, NULL);
		mvchgat(rrow, rcol + csr - dstart, 1, 0, GLOBAL_HIGHLIGHT_PAIR, NULL);

		refresh();
	}

	resp[resp_len] = 0;
	return resp;
}

void
prompt_complete_path(char **resp, size_t *resp_len, void *data)
{
}

static void
drawbox(char const *text)
{
	struct winsize tty_size;
	ioctl(0, TIOCGWINSZ, &tty_size);

	size_t text_size_y = 1, linewidth = 0;
	for (char const *c = text; *c; ++c) {
		if (*c == '\n' || ++linewidth > tty_size.ws_col) {
			linewidth = 0;
			++text_size_y;
		}
	}

	size_t box_top = tty_size.ws_row - text_size_y - 1;

	// clear box.
	for (size_t i = 1; i < tty_size.ws_row; ++i) {
		for (size_t j = 0; j < tty_size.ws_col; ++j)
			mvaddch(box_top + i, j, ' ');
	}

	// write separator.
	for (size_t i = 0; i < tty_size.ws_col; ++i)
		mvaddch(box_top, i, '-');

	// write actual text.
	mvaddstr(box_top + 1, 0, text);

	// set coloration.
	mvchgat(box_top, 0, tty_size.ws_col, 0, GLOBAL_HIGHLIGHT_PAIR, NULL);
	for (size_t i = 1; i < text_size_y + 1; ++i)
		mvchgat(box_top + i, 0, tty_size.ws_col, 0, GLOBAL_NORM_PAIR, NULL);

	refresh();
}
