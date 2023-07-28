#include "editor.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <ncurses.h>
#include <sys/ioctl.h>

#include "buf.h"
#include "def.h"
#include "frame.h"
#include "keybd.h"
#include "prompt.h"
#include "util.h"

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
	prompt_show("this keybind is not implemented yet!");
}

static void
bind_open_file(void)
{
	char *path = prompt_ask("open file:", prompt_complete_path, NULL);
	if (path)
		free(path);
}

static void
bind_save_file(void)
{
	prompt_show("this keybind is not implemented yet!");
}

static void
bind_navfwd_ch(void)
{
	frame_relmove_cursor(frames.data[cur_frame], 1, 0, true);
}

static void
bind_navfwd_word(void)
{
	prompt_show("this keybind is not implemented yet!");
}

static void
bind_navback_ch(void)
{
	frame_relmove_cursor(frames.data[cur_frame], -1, 0, true);
}

static void
bind_navback_word(void)
{
	prompt_show("this keybind is not implemented yet!");
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
bind_navgoto(void)
{
	prompt_show("this keybind is not implemented yet!");
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

	init_pair(GLOBAL_NORM_PAIR, GLOBAL_NORM_FG, GLOBAL_NORM_BG);
	init_pair(GLOBAL_HIGHLIGHT_PAIR, GLOBAL_HIGHLIGHT_FG, GLOBAL_HIGHLIGHT_BG);
	
	keybd_init();
	keybd_bind("^X ^C ", bind_quit);
	keybd_bind("^X b ", bind_chg_frame);
	keybd_bind("^X ^F ", bind_open_file);
	keybd_bind("^X ^S ", bind_save_file);
	keybd_bind("^F ", bind_navfwd_ch);
	keybd_bind("^[ f ", bind_navfwd_word);
	keybd_bind("^B ", bind_navback_ch);
	keybd_bind("^[ b ", bind_navback_word);
	keybd_bind("^N ", bind_navdown);
	keybd_bind("^P ", bind_navup);
	keybd_bind("^A ", bind_navln_start);
	keybd_bind("^E ", bind_navln_end);
	keybd_bind("^[ g ^[ g ", bind_navgoto);
	keybd_bind("^? ", bind_del);

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

	struct winsize tty_size;
	ioctl(0, TIOCGWINSZ, &tty_size);
	struct frame dframe = frame_create("*dummy*", 0, 0, tty_size.ws_col,
	                                   tty_size.ws_row, bufs.data[0],
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

	endwin();
	return;
	
	arraylist_destroy(&frames);
	arraylist_destroy(&frame_themes);
	arraylist_destroy(&bufs);
	
	keybd_quit();
	endwin();
}
