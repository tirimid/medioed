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
	uint16_t a;
};

static void sigwinch_handler(int arg);

static uint8_t const attrtab[] = {
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

static struct termios oldtios;
static struct cell **cells;
static struct winsize ws;

int
draw_init(void)
{
	if (tcgetattr(STDOUT_FILENO, &oldtios))
		return 1;

	struct termios new = oldtios;
	new.c_lflag &= ~ECHO;
	if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &new))
		return 1;

	fputws(L"\033[?25l", stdout);

	ioctl(0, TIOCGWINSZ, &ws);
	
	cells = malloc(sizeof(struct cell *) * ws.ws_row);
	for (size_t i = 0; i < ws.ws_row; ++i)
		cells[i] = malloc(sizeof(struct cell) * ws.ws_col);

	struct sigaction sa;
	sigaction(SIGWINCH, NULL, &sa);
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);
	
	return 0;
}

int
draw_quit(void)
{
	for (size_t i = 0; i < ws.ws_row; ++i)
		free(cells[i]);
	free(cells);
	
	fputws(L"\033[?25h", stdout);
	
	if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &oldtios))
		return 1;
	
	return 0;
}

void
draw_clear(wchar_t wch, uint16_t a)
{
	draw_fill(0, 0, ws.ws_row, ws.ws_col, wch, a);
}

void
draw_fill(unsigned pr, unsigned pc, unsigned sr, unsigned sc, wchar_t wch, uint16_t a)
{
	for (size_t i = pr; i < pr + sr; ++i) {
		for (size_t j = pc; j < pc + sc; ++j)
			draw_putwch(i, j, wch, a);
	}
}

void
draw_putwch(unsigned r, unsigned c, wchar_t wch, uint16_t a)
{
	if (r >= ws.ws_row || c >= ws.ws_col)
		return;
	
	cells[r][c] = (struct cell){
		.wch = wch,
		.a = a,
	};
}

void
draw_putwstr(unsigned r, unsigned c, wchar_t const *wstr, uint16_t a)
{
	for (wchar_t const *wc = wstr; *wc; ++wc) {
		draw_putwch(r, c, *wc, a);

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
	uint8_t prevattr = 0xff, prevfg = 0xff, prevbg = 0xff;

	fwprintf(stdout, L"\033[H");
	
	for (size_t i = 0; i < ws.ws_row; ++i) {
		for (size_t j = 0; j < ws.ws_col; ++j) {
			uint8_t curattr = A_TAOF(cells[i][j].a);
			uint8_t curfg = A_FGOF(cells[i][j].a) >> 3;
			uint8_t curbg = A_BGOF(cells[i][j].a) >> 8;
			
			if (curattr != prevattr)
				wprintf(L"\033[%um", attrtab[curattr - 1]);
			
			if (curfg != prevfg)
				wprintf(L"\033[%um", attrtab[curfg + 6]);

			if (curbg != prevbg)
				wprintf(L"\033[%um", attrtab[curbg + 14]);

			fputwc(cells[i][j].wch, stdout);

			prevattr = curattr;
			prevfg = curfg;
			prevbg = curbg;
		}
	}
}

static void
sigwinch_handler(int arg)
{
	struct winsize oldws = ws;
	ioctl(0, TIOCGWINSZ, &ws);

	if (ws.ws_row < oldws.ws_row) {
		for (size_t i = ws.ws_row; i < oldws.ws_row; ++i)
			free(cells[i]);
		cells = realloc(cells, sizeof(struct cell *) * ws.ws_row);
	} else if (ws.ws_row > oldws.ws_row) {
		for (size_t i = oldws.ws_row; i < ws.ws_row; ++i)
			cells[i] = malloc(sizeof(struct cell) * ws.ws_col);
	}

	if (ws.ws_col != oldws.ws_col) {
		for (size_t i = 0; i < MIN(oldws.ws_row, ws.ws_row); ++i)
			cells[i] = realloc(cells[i], sizeof(struct cell) * ws.ws_col);
	}
}
