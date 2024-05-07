#ifndef DRAW_H
#define DRAW_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

struct win_size
{
	unsigned sr, sc;
};

void draw_init(void);
void draw_quit(void);
void draw_clear(wchar_t wch, uint8_t fg, uint8_t bg);
void draw_fill(unsigned pr, unsigned pc, unsigned sr, unsigned sc, wchar_t wch, uint8_t fg, uint8_t bg);
void draw_put_wch(unsigned r, unsigned c, wchar_t wch);
void draw_put_wstr(unsigned r, unsigned c, wchar_t const *wstr);
void draw_put_attr(unsigned r, unsigned c, uint8_t fg, uint8_t bg, unsigned n);
void draw_refresh(void);
struct win_size draw_win_size(void);

#endif
