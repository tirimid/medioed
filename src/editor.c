#include "editor.h"

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#include <ncurses.h>
#include <tmcul/ds/arraylist.h>

#include "keybd.h"
#include "frame.h"
#include "buf.h"

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
	frame_relmove_cursor(frames.data[cur_frame], 1, 0, true);
}

static void
bind_navfwd_word(void)
{
}

static void
bind_navback_ch(void)
{
	frame_relmove_cursor(frames.data[cur_frame], -1, 0, true);
}

static void
bind_navback_word(void)
{
}

static void
bind_navdown(void)
{
	frame_relmove_cursor(frames.data[cur_frame], 0, 1, false);
}

static void
bind_navup(void)
{
	frame_relmove_cursor(frames.data[cur_frame], 0, -1, false);
}

static void
bind_navln_start(void)
{
	frame_relmove_cursor(frames.data[cur_frame], -INT_MAX, 0, false);
}

static void
bind_navln_end(void)
{
	frame_relmove_cursor(frames.data[cur_frame], INT_MAX, 0, false);
}

static void
bind_del(void)
{
	struct frame *f = frames.data[cur_frame];
	
	if (f->cursor > 0) {
		buf_erase(f->buf, f->cursor - 1, f->cursor);
		frame_relmove_cursor(f, -1, 0, true);
	}
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
	keybd_bind("^X\a^C\a", bind_quit);
	keybd_bind("^X\ab\a", bind_chg_frame);
	keybd_bind("^F\a", bind_navfwd_ch);
	keybd_bind("^[f\a", bind_navfwd_word);
	keybd_bind("^B\a", bind_navback_ch);
	keybd_bind("^[b\a", bind_navback_word);
	keybd_bind("^N\a", bind_navdown);
	keybd_bind("^P\a", bind_navup);
	keybd_bind("^A\a", bind_navln_start);
	keybd_bind("^E\a", bind_navln_end);
	keybd_bind("^?\a", bind_del);

	frames = arraylist_create();
	frame_themes = arraylist_create();
	bufs = arraylist_create();
	cur_frame = 0;

	// create dummy frame for testing.
	struct buf dbuf = buf_create();
	buf_write_str(&dbuf, 0, "#include <stdio.h>\n\nint\nmain(void)\n{\n\tprintf(\"hi\\n\");\n\treturn 0;\n}\n");
	arraylist_add(&bufs, &dbuf, sizeof(dbuf));
	
	struct frame_theme dtheme = frame_theme_default();
	arraylist_add(&frame_themes, &dtheme, sizeof(dtheme));
	
	struct frame dframe = frame_create("*dummy*", 4, 4, 20, 20, bufs.data[0],
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
		default: {
			struct frame *f = frames.data[cur_frame];
			buf_write_ch(f->buf, f->cursor, key);
			frame_relmove_cursor(f, 1, 0, true);
			break;
		}
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
