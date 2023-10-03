#include "editor.h"

#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "buf.h"
#include "conf.h"
#include "frame.h"
#include "keybd.h"
#include "mode.h"
#include "prompt.h"
#include "util.h"

static struct buf *addbuf(struct buf *b);
static struct frame *addframe(struct frame *f);
static void arrangeframes(void);
static void drawframes(void);
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
static void bind_del_ch(void);
static void bind_del_word(void);
static void bind_chgmode_global(void);
static void bind_chgmode_local(void);
static void bind_create_scrap(void);
static void resetbinds(void);
static void sigwinch_handler(int arg);

static bool running;
static size_t cur_frame;
static struct arraylist frames;
static struct arraylist bufs;
static void (*old_sigwinch_handler)(int);

int
editor_init(int argc, char const *argv[])
{
	initscr();
	start_color();
	raw();
	noecho();
	curs_set(0);

	if (conf_init() != 0) {
		endwin();
		fputs("failed on conf_init()!\n", stderr);
		return 1;
	}

	keybd_init();

	frames = arraylist_create();
	bufs = arraylist_create();
	cur_frame = 0;

	if (argc <= 1) {
		struct buf gb = buf_from_str(CONF_GREET_TEXT, false);
		struct buf *gbp = addbuf(&gb);
		struct frame gf = frame_create("*greeter*", gbp);
		addframe(&gf);
	} else {
		for (int i = 1; i < argc; ++i) {
			struct stat s;
			if (stat(argv[i], &s) || !S_ISREG(s.st_mode)) {
				char *msg = malloc(strlen(argv[i]) + 22);
				sprintf(msg, "could not open file: %s", argv[i]);

				clear();
				prompt_show(msg);

				free(msg);
				continue;
			}

			struct buf b = buf_from_file(argv[i], true);
			struct buf *bp = addbuf(&b);
			struct frame f = frame_create(argv[i], bp);
			addframe(&f);
		}
	}

	struct sigaction sa;
	sigaction(SIGWINCH, NULL, &sa);
	old_sigwinch_handler = sa.sa_handler;

	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);

	resetbinds();
	arrangeframes();

	return 0;
}

void
editor_main_loop(void)
{
	running = true;

	while (running) {
		// ensure frames sharing buffers are in a valid state.
		for (size_t i = 0; i < frames.size; ++i) {
			struct frame *f = frames.data[i];
			f->cursor = MIN(f->cursor, f->buf->size);
		}

		drawframes();
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
			mode_keyupdate(f, key);

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

	for (size_t i = 0; i < bufs.size; ++i)
		buf_destroy(bufs.data[i]);

	arraylist_destroy(&frames);
	arraylist_destroy(&bufs);

	keybd_quit();
	conf_quit();
	endwin();
}

static struct buf *
addbuf(struct buf *b)
{
	for (size_t i = 0; i < bufs.size; ++i) {
		struct buf *ob = bufs.data[i];

		if (b->src_type == BUF_SRC_TYPE_FILE
		    && ob->src_type == BUF_SRC_TYPE_FILE
		    && !strcmp(b->src, ob->src)) {
			buf_destroy(b);
			return ob;
		}
	}

	arraylist_add(&bufs, b, sizeof(*b));
	return bufs.data[bufs.size - 1];
}

static struct frame *
addframe(struct frame *f)
{
	arraylist_add(&frames, f, sizeof(*f));
	return frames.data[frames.size - 1];
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
drawframes(void)
{
	// rather than calling `clear()`, manually wipe the screen.
	// this reduces tearing in a TTY.
	struct winsize tty_size;
	ioctl(0, TIOCGWINSZ, &tty_size);
	for (unsigned i = 0; i < tty_size.ws_row; ++i) {
		mvchgat(i, 0, tty_size.ws_col, 0, conf_gnorm, NULL);
		for (unsigned j = 0; j < tty_size.ws_col; ++j)
			mvaddch(i, j, ' ');
	}

	for (size_t i = 0; i < frames.size; ++i)
		frame_draw(frames.data[i], i == cur_frame);
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
	char *path = prompt_ask("kill active frame? (y/N) ", NULL, NULL);
	if (!path)
		return;

	if (!strcmp(path, "y")) {
		frame_destroy(frames.data[cur_frame]);
		arraylist_rm(&frames, cur_frame);
		cur_frame = cur_frame > 0 ? cur_frame - 1 : 0;

		// destroy orphaned buffers.
		for (size_t i = 0; i < bufs.size; ++i) {
			bool orphan = true;
			for (size_t j = 0; j < frames.size; ++j) {
				struct frame const *f = frames.data[j];
				if (bufs.data[i] == f->buf) {
					orphan = false;
					break;
				}
			}

			if (orphan) {
				buf_destroy(bufs.data[i]);
				arraylist_rm(&bufs, i);
				--i;
			}
		}

		resetbinds();
		arrangeframes();

		free(path);
	} else if (!strcmp(path, "n") || !*path)
		free(path);
	else {
		free(path);
		prompt_show("expected either 'y' or 'n'!");
		goto ask_again;
	}

	if (frames.size == 0) {
		struct buf b = buf_create(true);
		struct frame f = frame_create("*scratch*", addbuf(&b));
		addframe(&f);
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
	struct frame frame = frame_create(path, addbuf(&buf));
	addframe(&frame);

	cur_frame = frames.size - 1;
	resetbinds();
	arrangeframes();

	free(path);
}

static void
bind_save_file(void)
{
	struct frame *f = frames.data[cur_frame];
	enum buf_src_type prevtype = f->buf->src_type;

	if (f->buf->src_type == BUF_SRC_TYPE_FRESH) {
		f->buf->src_type = BUF_SRC_TYPE_FILE;
		f->buf->src = prompt_ask("save to file: ", prompt_complete_path, NULL);;
	}

	// no path was given as source for new buffer, so the type is reset.
	if (!f->buf->src) {
		f->buf->src_type = prevtype;
		return;
	}

	if (buf_save(f->buf)) {
		prompt_show("failed to write file!");
		return;
	}

	if (prevtype == BUF_SRC_TYPE_FRESH) {
		free(f->name);
		f->name = strdup(f->buf->src);
	}

	if (!f->localmode[0]) {
		free(f->localmode);
		f->localmode = strdup(fileext(f->buf->src));
	}
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

	if (!*linum_text) {
		free(linum_text);
		prompt_show("expected a line number!");
		goto ask_again;
	}

	for (char const *c = linum_text; *c; ++c) {
		if (!isdigit(*c)) {
			free(linum_text);
			prompt_show("invalid line number!");
			goto ask_again;
		}
	}

	unsigned linum = atoi(linum_text);
	linum -= linum != 0;
	free(linum_text);

	frame_move_cursor(frames.data[cur_frame], 0, linum);
}

static void
bind_del_ch(void)
{
	struct frame *f = frames.data[cur_frame];

	if (f->cursor > 0 && f->buf->writable) {
		buf_erase(f->buf, f->cursor - 1, f->cursor);
		frame_relmove_cursor(f, -1, 0, true);
	}
}

static void
bind_del_word(void)
{
	prompt_show("this keybind is not implemented yet!");
}

static void
bind_chgmode_global(void)
{
	char *new_gm = prompt_ask("new globalmode: ", NULL, NULL);
	if (!new_gm)
		return;

	mode_set(new_gm, frames.data[cur_frame]);
	free(new_gm);
}

static void
bind_chgmode_local(void)
{
	char *new_lm = prompt_ask("new frame localmode: ", NULL, NULL);
	if (!new_lm)
		return;

	struct frame *f = frames.data[cur_frame];
	free(f->localmode);
	f->localmode = new_lm;
}

static void
bind_create_scrap(void)
{
	prompt_show("this keybind is not implemented yet!");
}

static void
resetbinds(void)
{
	// quit and reinit to reset current keybind buffer and bind information.
	keybd_quit();
	keybd_init();

	keybd_bind(CONF_BIND_QUIT, bind_quit);
	keybd_bind(CONF_BIND_CHGFWD_FRAME, bind_chgfwd_frame);
	keybd_bind(CONF_BIND_CHGBACK_FRAME, bind_chgback_frame);
	keybd_bind(CONF_BIND_FOCUS_FRAME, bind_focus_frame);
	keybd_bind(CONF_BIND_KILL_FRAME, bind_kill_frame);
	keybd_bind(CONF_BIND_OPEN_FILE, bind_open_file);
	keybd_bind(CONF_BIND_SAVE_FILE, bind_save_file);
	keybd_bind(CONF_BIND_NAVFWD_CH, bind_navfwd_ch);
	keybd_bind(CONF_BIND_NAVFWD_WORD, bind_navfwd_word);
	keybd_bind(CONF_BIND_NAVBACK_CH, bind_navback_ch);
	keybd_bind(CONF_BIND_NAVBACK_WORD, bind_navback_word);
	keybd_bind(CONF_BIND_NAVDOWN, bind_navdown);
	keybd_bind(CONF_BIND_NAVUP, bind_navup);
	keybd_bind(CONF_BIND_NAVLN_START, bind_navln_start);
	keybd_bind(CONF_BIND_NAVLN_END, bind_navln_end);
	keybd_bind(CONF_BIND_NAVGOTO, bind_navgoto);
	keybd_bind(CONF_BIND_DEL_CH, bind_del_ch);
	keybd_bind(CONF_BIND_DEL_WORD, bind_del_word);
	keybd_bind(CONF_BIND_CHGMODE_GLOBAL, bind_chgmode_global);
	keybd_bind(CONF_BIND_CHGMODE_LOCAL, bind_chgmode_local);
	keybd_bind(CONF_BIND_CREATE_SCRAP, bind_create_scrap);
}

static void
sigwinch_handler(int arg)
{
	old_sigwinch_handler(arg);

	arrangeframes();
	drawframes();
	refresh();
}
