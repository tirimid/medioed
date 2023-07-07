#include "prompt.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>
#include <sys/ioctl.h>

static void
drawbox(char const *text)
{
	struct winsize tty_size;
	ioctl(0, TIOCGWINSZ, &tty_size);

	int text_size_y = 1, linewidth = 0;
	for (char const *c = text; *c; ++c) {
		if (*c == '\n' || ++linewidth > tty_size.ws_row) {
			linewidth = 0;
			++text_size_y;
		}
	}

	size_t box_top = tty_size.ws_row - text_size_y - 1;

	// clear box.
	for (size_t i = 0; i < tty_size.ws_row; ++i) {
		for (size_t j = 0; j < tty_size.ws_col; ++j)
			mvaddch(box_top + i + 1, j, ' ');
	}

	// write separator.
	for (size_t i = 0; i < tty_size.ws_col; ++i)
		mvaddch(box_top, i, '-');

	// write actual text.
	mvaddstr(box_top + 1, 0, text);

	// set coloration.
	for (size_t i = 0; i < text_size_y + 1; ++i)
		mvchgat(box_top + i, 0, tty_size.ws_col, 0, 1, NULL);
}

void
prompt_show(char const *text)
{
	char *fulltext = malloc(strlen(text) + 24);
	sprintf(fulltext, "%s\n\npress 'q' to close...", text);
	drawbox(fulltext);
	free(fulltext);
	
	int k;
	while ((k = getch()) != 'q' && k != 'Q');
}

char *
prompt_ask(char const *text)
{
	char *fulltext = malloc(strlen(text) + 4);
	sprintf(fulltext, "%s\n=>", text);
	drawbox(fulltext);
	free(fulltext);

	return NULL;
}

char *
prompt_ask_buf(char out_buf[], size_t n, char const *text)
{
	char *fulltext = malloc(strlen(text) + 4);
	sprintf(fulltext, "%s\n=>", text);
	drawbox(fulltext);
	free(fulltext);
	
	return out_buf;
}
