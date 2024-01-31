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
	uint16_t attr;
};

static void sigwinch_handler(int arg);

static uint8_t const attr_tab[] = {
	// text attributes.
	0,
	1,
	2,
	4,
	5,
	7,
	8,

	// foreground color.
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,

	// background color.
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
};

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
draw_clear(wchar_t wch, uint16_t a)
{
	draw_fill(0, 0, ws.ws_row, ws.ws_col, wch, a);
}

void
draw_fill(unsigned pr, unsigned pc, unsigned sr, unsigned sc, wchar_t wch,
          uint16_t a)
{
	for (size_t i = pr; i < pr + sr; ++i) {
		for (size_t j = pc; j < pc + sc; ++j) {
			cells[i][j] = (struct cell){
				.wch = wch,
				.attr = a,
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
draw_put_attr(unsigned r, unsigned c, uint16_t a, unsigned n)
{
	if (r >= ws.ws_row || c >= ws.ws_col)
		return;
	
	for (unsigned i = 0; i < n; ++i) {
		cells[r][c].attr = a;

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
	uint8_t prev_attr = 0xff, prev_fg = 0xff, prev_bg = 0xff;

	fwprintf(stdout, L"\033[H");
	
	for (size_t i = 0; i < ws.ws_row; ++i) {
		for (size_t j = 0; j < ws.ws_col; ++j) {
			uint8_t cur_attr = A_NC_ATTR_OF(cells[i][j].attr);
			uint8_t cur_fg = A_FG_OF(cells[i][j].attr) >> 3;
			uint8_t cur_bg = A_BG_OF(cells[i][j].attr) >> 8;
			
			if (cur_attr != prev_attr
			    || cur_fg != prev_fg
			    || cur_bg != prev_bg) {
				wprintf(L"\033[0m\033[%u;%u;%um",
				        attr_tab[cur_attr],
				        attr_tab[cur_fg + 6],
				        attr_tab[cur_bg + 14]);
			}
			
			fputwc(cells[i][j].wch, stdout);

			prev_attr = cur_attr;
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
