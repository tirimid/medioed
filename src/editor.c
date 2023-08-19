#include "editor.h"

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <ncurses.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "buf.h"
#include "def.h"
#include "frame.h"
#include "keybd.h"
#include "prompt.h"
#include "util.h"

static void resetbinds(void);
static void arrangeframes(void);
static void bind_quit(void);
static void bind_chgfwd_frame(void);
static void bind_chgback_frame(void);
static void bind_focus_frame(void);
static void bind_kill_frame(void);
static void bind_open_file(void);
static void bind_save_file(void);
static void bind_navfwd_ch(void);
static void bind_navfwd_word(void);
static void bind_navback_ch(void);
static void bind_navback_word(void);
static void bind_navdown(void);
static void bind_navup(void);
static void bind_navln_start(void);
static void bind_navln_end(void);
static void bind_navgoto(void);
static void bind_del(void);
static void resetbinds(void);

static bool running;
static size_t cur_frame;
static struct arraylist frames, frame_themes;
static struct arraylist bufs;

void
editor_init(int argc, char const *argv[])
{
	initscr();
	start_color();
	raw();
	noecho();
	curs_set(0);

	init_pair(GLOBAL_NORM_PAIR, GLOBAL_NORM_FG, GLOBAL_NORM_BG);
	init_pair(GLOBAL_HIGHLIGHT_PAIR, GLOBAL_HIGHLIGHT_FG, GLOBAL_HIGHLIGHT_BG);
	
	keybd_init();

	frames = arraylist_create();
	frame_themes = arraylist_create();
	bufs = arraylist_create();
	cur_frame = 0;

	struct frame_theme deftheme = frame_theme_default();
	arraylist_add(&frame_themes, &deftheme, sizeof(deftheme));

	struct buf greet_buf = buf_from_str(GLOBAL_GREET_TEXT, false);
	arraylist_add(&bufs, &greet_buf, sizeof(greet_buf));
	
	struct frame greet_frame = frame_create("*greeter*", bufs.data[0],
	                                        frame_themes.data[0]);
	arraylist_add(&frames, &greet_frame, sizeof(greet_frame));

	resetbinds();
}

void
editor_main_loop(void)
{
	running = true;
	
	while (running) {
		if (frames.size > 0) {
			for (size_t i = 0; i < frames.size; ++i)
				frame_draw(frames.data[i], i == cur_frame);
		} else {
			struct winsize tty_size;
			ioctl(0, TIOCGWINSZ, &tty_size);

			// clear display characters.
			for (unsigned i = 0; i < tty_size.ws_row; ++i) {
				for (unsigned j = 0; j < tty_size.ws_col; ++j)
					mvaddch(i, j, ' ');
			}

			// set base display attributes.
			for (unsigned i = 0; i < tty_size.ws_row; ++i)
				mvchgat(i, 0, tty_size.ws_col, 0, GLOBAL_NORM_PAIR, NULL);
		}

		refresh();
		
		int key = keybd_await_input();
		switch (key) {
		case KEYBD_IGNORE_BIND:
			break;
		default: {
			if (frames.size == 0)
				break;
			
			struct frame *f = frames.data[cur_frame];
			buf_write_ch(f->buf, f->cursor, key);
			frame_relmove_cursor(f, f->buf->writable, 0, true);
			
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

static void
arrangeframes(void)
{
	if (frames.size == 0)
		return;
	
	struct winsize tty_size;
	ioctl(0, TIOCGWINSZ, &tty_size);

	struct frame *f = frames.data[0];
	
	f->pos_x = f->pos_y = 0;
	f->size_y = tty_size.ws_row;
	f->size_x = frames.size == 1 ? tty_size.ws_col : 4 * tty_size.ws_col / 7;

	for (size_t i = 1; i < frames.size; ++i) {
		f = frames.data[i];

		f->size_y = tty_size.ws_row / (frames.size - 1);
		f->pos_y = (i - 1) * f->size_y;
		f->pos_x = 4 * tty_size.ws_col / 7;
		f->size_x = tty_size.ws_col - f->pos_x;

		if (f->pos_y + f->size_y > tty_size.ws_row)
			f->size_y = tty_size.ws_row - f->pos_y;
	}
}

static void
bind_quit(void)
{
	running = false;
}

static void
bind_chgfwd_frame(void)
{
	cur_frame = (cur_frame + 1) % frames.size;
	resetbinds();
}

static void
bind_chgback_frame(void)
{
	cur_frame = (cur_frame == 0 ? frames.size : cur_frame) - 1;
	resetbinds();
}

static void
bind_focus_frame(void)
{
	arraylist_swap(&frames, 0, cur_frame);
	cur_frame = 0;
	arrangeframes();
}

static void
bind_kill_frame(void)
{
ask_again:;
	char *path = prompt_ask("kill active frame? (y/n) ", NULL, NULL);
	if (!path)
		return;

	if (!strcmp(path, "y")) {
		// TODO: add checks for orphaned frame themes and buffers.
		
		frame_destroy(frames.data[cur_frame]);
		arraylist_rm(&frames, cur_frame);
		cur_frame = cur_frame > 0 ? cur_frame - 1 : 0;

		resetbinds();
		arrangeframes();
			
		free(path);
	} else if (!strcmp(path, "n"))
		free(path);
	else {
		free(path);
		prompt_show("expected either 'y' or 'n'!");
		goto ask_again;
	}
}

static void
bind_open_file(void)
{
	char *path = prompt_ask("open file: ", prompt_complete_path, NULL);
	if (!path)
		return;

	struct stat s;
	if (stat(path, &s) || !S_ISREG(s.st_mode)) {
		prompt_show("could not open file!");
		free(path);
		return;
	}

	struct buf buf = buf_from_file(path, true);
	arraylist_add(&bufs, &buf, sizeof(buf));
	
	struct frame frame = frame_create(path, bufs.data[bufs.size - 1],
	                                  frame_themes.data[0]);
	arraylist_add(&frames, &frame, sizeof(frame));

	cur_frame = frames.size - 1;
	resetbinds();
	arrangeframes();

	free(path);
}

static void
bind_save_file(void)
{
	struct frame *f = frames.data[cur_frame];
	struct buf *b = f->buf;

	if (b->src_type == BUF_SRC_TYPE_FRESH) {
		b->src_type = BUF_SRC_TYPE_FILE;
		b->src = prompt_ask("save to file: ", prompt_complete_path, NULL);
	}

	// no path was given as source for new buffer, so it is reset to `FRESH`.
	if (!b->src) {
		b->src_type = BUF_SRC_TYPE_FRESH;
		return;
	}
	
	if (buf_save(b))
		prompt_show("failed to write file!");
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
ask_again:;
	char *linum_text = prompt_ask("goto line: ", NULL, NULL);
	if (!linum_text)
		return;

	for (char const *c = linum_text; *c; ++c) {
		if (!isdigit(*c)) {
			free(linum_text);
			prompt_show("expected a valid line number!");
			goto ask_again;
		}
	}

	unsigned linum = atoi(linum_text);
	linum -= linum != 0;
	free(linum_text);

	frame_move_cursor(frames.data[cur_frame], 0, linum);
}

static void
bind_del(void)
{
	struct frame *f = frames.data[cur_frame];
	
	if (f->cursor > 0 && f->buf->writable) {
		buf_erase(f->buf, f->cursor - 1, f->cursor);
		frame_relmove_cursor(f, -1, 0, true);
	}
}

static void
resetbinds(void)
{
	// quit and reinit to reset current keybind buffer and bind information.
	keybd_quit();
	keybd_init();
	
	keybd_bind(GLOBAL_BIND_QUIT, bind_quit);
	keybd_bind(GLOBAL_BIND_CHGFWD_FRAME, bind_chgfwd_frame);
	keybd_bind(GLOBAL_BIND_CHGBACK_FRAME, bind_chgback_frame);
	keybd_bind(GLOBAL_BIND_FOCUS_FRAME, bind_focus_frame);
	keybd_bind(GLOBAL_BIND_KILL_FRAME, bind_kill_frame);
	keybd_bind(GLOBAL_BIND_OPEN_FILE, bind_open_file);
	keybd_bind(GLOBAL_BIND_SAVE_FILE, bind_save_file);
	keybd_bind(GLOBAL_BIND_NAVFWD_CH, bind_navfwd_ch);
	keybd_bind(GLOBAL_BIND_NAVFWD_WORD, bind_navfwd_word);
	keybd_bind(GLOBAL_BIND_NAVBACK_CH, bind_navback_ch);
	keybd_bind(GLOBAL_BIND_NAVBACK_WORD, bind_navback_word);
	keybd_bind(GLOBAL_BIND_NAVDOWN, bind_navdown);
	keybd_bind(GLOBAL_BIND_NAVUP, bind_navup);
	keybd_bind(GLOBAL_BIND_NAVLN_START, bind_navln_start);
	keybd_bind(GLOBAL_BIND_NAVLN_END, bind_navln_end);
	keybd_bind(GLOBAL_BIND_NAVGOTO, bind_navgoto);
	keybd_bind(GLOBAL_BIND_DEL, bind_del);
}
