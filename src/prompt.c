#include "prompt.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>
#include <sys/ioctl.h>

#include "def.h"

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
}

void
prompt_show(char const *text)
{
	drawbox(text);
	
	int k;
	while ((k = getch()) != 'q' && k != 'Q');
}

char *
prompt_ask(char const *text)
{
	drawbox(text);
	return NULL;
}
