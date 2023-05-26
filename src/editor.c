#include "editor.h"

#include <stdbool.h>
#include <stddef.h>

#include <ncurses.h>
#include <libtmcul/ds/arraylist.h>

#include "keybd.h"
#include "frame.h"

static bool running;
static size_t cur_frame;
static struct arraylist frames, frame_themes;
static struct arraylist bufs;

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
	struct frame *f = frames.data[cur_frame];
	if (f->cursor < f->buf->size)
		++f->cursor;
}

static void
bind_navfwd_word(void)
{
}

static void
bind_navback_ch(void)
{
	struct frame *f = frames.data[cur_frame];
	if (f->cursor > 0)
		--f->cursor;
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
	curs_set(0);

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

	frames = arraylist_create();
	frame_themes = arraylist_create();
	bufs = arraylist_create();
	cur_frame = 0;

	// create dummy frame for testing.
	struct buf dbuf = buf_create();
	buf_write_str(&dbuf, 0, "hello world");
	arraylist_add(&bufs, &dbuf, sizeof(dbuf));
	
	struct frame_theme dtheme = frame_theme_default();
	arraylist_add(&frame_themes, &dtheme, sizeof(dtheme));
	
	struct frame dframe = frame_create("*dummy*", 0, 0, 20, 20, bufs.data[0],
	                                   frame_themes.data[0]);
	arraylist_add(&frames, &dframe, sizeof(dframe));
}

static bool
frame_visible(size_t ind)
{
	return true;
}

void
editor_main_loop(void)
{
	running = true;
	while (running) {
		for (size_t i = 0; i < frames.size; ++i) {
			if (frame_visible(i))
				frame_draw(frames.data[i]);
		}

		refresh();
		
		int key = keybd_await_input();
		switch (key) {
		case KEYBD_IGNORE_BIND:
			break;
		default:
			break;
		}
	}
}

void
editor_quit(void)
{
	for (size_t i = 0; i < frames.size; ++i)
		frame_destroy(frames.data[i]);

	for (size_t i = 0; i < frame_themes.size; ++i)
		frame_theme_destroy(frame_themes.data[i]);

	for (size_t i = 0; i < bufs.size; ++i)
		buf_destroy(bufs.data[i]);
	
	arraylist_destroy(&frames);
	arraylist_destroy(&frame_themes);
	arraylist_destroy(&bufs);
	
	keybd_quit();
	endwin();
}
