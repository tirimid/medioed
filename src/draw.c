#include "draw.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "util.h"

struct cell {
	wchar_t wch;
	uint8_t fg, bg;
};

static void sigwinch_handler(int arg);

static struct cell **cells;
static struct winsize ws;

void
draw_init(void)
{
	fputws(L"\033[?25l", stdout);

	ioctl(0, TIOCGWINSZ, &ws);
	
	cells = malloc(sizeof(struct cell *) * ws.ws_row);
	for (size_t i = 0; i < ws.ws_row; ++i)
		cells[i] = malloc(sizeof(struct cell) * ws.ws_col);

	struct sigaction sa;
	sigaction(SIGWINCH, NULL, &sa);
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);
}

void
draw_quit(void)
{
	for (size_t i = 0; i < ws.ws_row; ++i)
		free(cells[i]);
	free(cells);
	
	fputws(L"\033[?25h", stdout);
}

void
draw_clear(wchar_t wch, uint8_t fg, uint8_t bg)
{
	draw_fill(0, 0, ws.ws_row, ws.ws_col, wch, fg, bg);
}

void
draw_fill(unsigned pr, unsigned pc, unsigned sr, unsigned sc, wchar_t wch,
          uint8_t fg, uint8_t bg)
{
	for (size_t i = pr; i < pr + sr; ++i) {
		for (size_t j = pc; j < pc + sc; ++j) {
			cells[i][j] = (struct cell){
				.wch = wch,
				.fg = fg,
				.bg = bg,
			};
		}
	}
}

void
draw_put_wch(unsigned r, unsigned c, wchar_t wch)
{
	cells[r][c].wch = wch;
}

void
draw_put_wstr(unsigned r, unsigned c, wchar_t const *wstr)
{
	for (wchar_t const *wc = wstr; *wc; ++wc) {
		if (*wc != L'\n')
			cells[r][c++].wch = *wc;

		if (c >= ws.ws_col || *wc == L'\n') {
			c = 0;
			++r;
		}

		if (r >= ws.ws_row)
			break;
	}
}

void
draw_put_attr(unsigned r, unsigned c, uint8_t fg, uint8_t bg, unsigned n)
{
	if (r >= ws.ws_row || c >= ws.ws_col)
		return;
	
	for (unsigned i = 0; i < n; ++i) {
		cells[r][c].fg = fg;
		cells[r][c].bg = bg;

		if (++c >= ws.ws_col) {
			c = 0;
			++r;
		}

		if (r >= ws.ws_row)
			break;
	}
}

void
draw_refresh(void)
{
	uint8_t prev_fg = 0xff, prev_bg = 0xff;

	fwprintf(stdout, L"\033[H\033[0m");
	
	for (size_t i = 0; i < ws.ws_row; ++i) {
		for (size_t j = 0; j < ws.ws_col; ++j) {
			uint8_t cur_fg = cells[i][j].fg;
			uint8_t cur_bg = cells[i][j].bg;
			
			if (cur_fg != prev_fg || cur_bg != prev_bg)
				wprintf(L"\033[38;5;%um\033[48;5;%um", cur_fg, cur_bg);
			
			fputwc(cells[i][j].wch, stdout);
			
			prev_fg = cur_fg;
			prev_bg = cur_bg;
		}
	}
}

static void
sigwinch_handler(int arg)
{
	struct winsize old_ws = ws;
	ioctl(0, TIOCGWINSZ, &ws);

	if (ws.ws_row > old_ws.ws_row) {
		cells = realloc(cells, sizeof(struct cell *) * ws.ws_row);
		for (size_t i = old_ws.ws_row; i < ws.ws_row; ++i)
			cells[i] = malloc(sizeof(struct cell) * ws.ws_col);
	}

	if (ws.ws_col > old_ws.ws_col) {
		for (size_t i = 0; i < old_ws.ws_row; ++i)
			cells[i] = realloc(cells[i], sizeof(struct cell) * ws.ws_col);
	}
}
