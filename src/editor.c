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

static void
bind_chg_frame(void)
{
}

static void
bind_navfwd_ch(void)
{
}

static void
bind_navfwd_word(void)
{
}

static void
bind_navback_ch(void)
{
}

static void
bind_navback_word(void)
{
}

static void
bind_navdown(void)
{
}

static void
bind_navup(void)
{
}

static void
bind_navln_start(void)
{
}

static void
bind_navln_end(void)
{
}

static void
bind_del(void)
{
}

void
editor_init(void)
{
	initscr();
	start_color();
	raw();
	noecho();

	keybd_init();
	keybd_bind("^X^C", bind_quit);
	keybd_bind("^Xb", bind_chg_frame);
	keybd_bind("^F", bind_navfwd_ch);
	keybd_bind("^[f", bind_navfwd_word);
	keybd_bind("^B", bind_navback_ch);
	keybd_bind("^[b", bind_navback_word);
	keybd_bind("^N", bind_navdown);
	keybd_bind("^P", bind_navup);
	keybd_bind("^A", bind_navln_start);
	keybd_bind("^E", bind_navln_end);
	keybd_bind("^?", bind_del);
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
