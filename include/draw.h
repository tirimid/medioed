#ifndef DRAW_H__
#define DRAW_H__

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#define A_TAOF(a) (a & 0x7)
#define A_FGOF(a) (a & 0x78)
#define A_BGOF(a) (a & 0xf80)

enum attr {
	A_BRIGHT = 1,
	A_DIM = 2,
	A_UNDERSCORE = 3,
	A_BLINK = 4,
	A_REVERSE = 5,
	A_HIDDEN = 6,
	
	A_BLACK = 1 << 3,
	A_RED = 2 << 3,
	A_GREEN = 3 << 3,
	A_YELLOW = 4 << 3,
	A_BLUE = 5 << 3,
	A_MAGENTA = 6 << 3,
	A_CYAN = 7 << 3,
	A_WHITE = 8 << 3,
	
	A_BBLACK = 1 << 8,
	A_BRED = 2 << 8,
	A_BGREEN = 3 << 8,
	A_BYELLOW = 4 << 8,
	A_BBLUE = 5 << 8,
	A_BMAGENA = 6 << 8,
	A_BCYAN = 7 << 8,
	A_BWHITE = 8 << 8,
};

int draw_init(void);
int draw_quit(void);
void draw_clear(void);
void draw_fill(unsigned pr, unsigned pc, unsigned sr, unsigned sc, wchar_t wch, uint16_t a);
void draw_putwch(unsigned r, unsigned c, wchar_t wch, uint16_t a);
void draw_putwstr(unsigned r, unsigned c, wchar_t const *wstr, uint16_t a);
void draw_putnwstr(unsigned r, unsigned c, wchar_t const *wstr, size_t n, uint16_t a);

#endif
