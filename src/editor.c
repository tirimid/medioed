#include "editor.h"

#include <stdbool.h>

#include <ncurses.h>

#include "keybd.h"
#include "frame.h"

static bool running;

static void
bind_quit(void)
{
	running = false;
}

void
editor_init(void)
{
	initscr();
	raw();
	noecho();

	keybd_init();
	keybd_bind("^X^C", bind_quit);
}

void
editor_main_loop(void)
{
	running = true;
	while (running) {
		int key = keybd_await_input();
		switch (key) {
		case KEYBD_IGNORE:
			break;
		default:
			addch(key);
			break;
		}
	}
}

void
editor_quit(void)
{
	keybd_quit();
	endwin();
}
