#include "draw.h"

#include <stdio.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static void attrset(uint16_t a);

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
	
	return 0;
}

int
draw_quit(void)
{
	fputws(L"\033[?25h", stdout);
	
	if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &oldtios))
		return 1;
	
	return 0;
}

void
draw_clear(void)
{
	fputws(L"\033[2J", stdout);
}

void
draw_fill(unsigned pr, unsigned pc, unsigned sr, unsigned sc, wchar_t wch, uint16_t a)
{
	attrset(a);
	for (unsigned i = 0; i < sr; ++i) {
		wprintf(L"\033[%u;%uH", pr + 1 + i, pc + 1);
		for (unsigned j = 0; j < sc; ++j)
			putwc(wch, stdout);
	}
}

void
draw_putwch(unsigned r, unsigned c, wchar_t wch, uint16_t a)
{
	attrset(a);
	wprintf(L"\033[%u;%uH", r + 1, c + 1);
	putwc(wch, stdout);
}

void
draw_putwstr(unsigned r, unsigned c, wchar_t const *wstr, uint16_t a)
{
	attrset(a);
	wprintf(L"\033[%u;%uH", r + 1, c + 1);
	fputws(wstr, stdout);
}

void
draw_putnwstr(unsigned r, unsigned c, wchar_t const *wstr, size_t n, uint16_t a)
{
	attrset(a);
	wprintf(L"\033[%u;%uH", r + 1, c + 1);
	for (size_t i = 0; wstr[i]; ++i)
		putwc(wstr[i], stdout);
}

static void
attrset(uint16_t a)
{
	fputws(L"\033[0m", stdout);
	
	uint8_t attr = A_TAOF(a);
	uint8_t fg = A_FGOF(a) >> 3;
	uint8_t bg = A_BGOF(a) >> 8;

	if (attr)
		wprintf(L"\033[%um", attrtab[attr - 1]);
	
	if (fg)
		wprintf(L"\033[%um", attrtab[fg + 6]);

	if (bg)
		wprintf(L"\033[%um", attrtab[bg + 14]);
}
