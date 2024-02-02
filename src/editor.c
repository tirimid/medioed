#include "editor.h"

#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "buf.h"
#include "conf.h"
#include "draw.h"
#include "frame.h"
#include "keybd.h"
#include "prompt.h"

extern bool flag_c;

static struct buf *add_buf(struct buf *b);
static struct frame *add_frame(struct frame *f);
static void arrange_frames(void);
static void bind_quit(void);
static void bind_chg_frame_fwd(void);
static void bind_chg_frame_back(void);
static void bind_focus_frame(void);
static void bind_kill_frame(void);
static void bind_open_file(void);
static void bind_save_file(void);
static void bind_nav_fwd_ch(void);
static void bind_nav_fwd_word(void);
static void bind_nav_fwd_page(void);
static void bind_nav_back_ch(void);
static void bind_nav_back_word(void);
static void bind_nav_back_page(void);
static void bind_nav_down(void);
static void bind_nav_up(void);
static void bind_nav_ln_start(void);
static void bind_nav_ln_end(void);
static void bind_nav_goto(void);
static void bind_del_fwd_ch(void);
static void bind_del_back_ch(void);
static void bind_del_back_word(void);
static void bind_chg_mode_global(void);
static void bind_chg_mode_local(void);
static void bind_create_scrap(void);
static void bind_new_line(void);
static void bind_focus(void);
static void bind_kill(void);
static void bind_paste(void);
static void bind_undo(void);
static void bind_copy(void);
static void bind_ncopy(void);
static void bind_find_lit(void);
static void bind_mac_begin(void);
static void bind_mac_end(void);
static void bind_toggle_mono(void);
static void reset_binds(void);
static void set_global_mode(void);
static void sigwinch_handler(int arg);

static void (*old_sigwinch_handler)(int);
static bool running;
static size_t cur_frame;
static struct vec_frame frames;
static struct vec_p_buf p_bufs;
static wchar_t *clipbuf = NULL;
static bool mono = false;

int
editor_init(int argc, char const *argv[])
{
	keybd_init();

	frames = vec_frame_create();
	p_bufs = vec_p_buf_create();
	cur_frame = 0;
	
	int first_arg = 1;
	while (first_arg < argc && *argv[first_arg] == '-')
		++first_arg;

	if (argc <= first_arg) {
		struct buf gb = buf_from_wstr(CONF_GREET_TEXT, false);
		struct frame gf = frame_create(CONF_GREET_NAME, add_buf(&gb));
		add_frame(&gf);
	} else {
		for (int i = first_arg; i < argc; ++i) {
			draw_clear(L' ', CONF_A_GNORM_FG, CONF_A_GNORM_BG);
			
			struct stat s;
			if ((stat(argv[i], &s) || !S_ISREG(s.st_mode)) && !flag_c) {
				// TODO: show which file failed to open.
				prompt_show(L"failed to open file!");
				continue;
			} else if (stat(argv[i], &s) && flag_c) {
				if (mk_file(argv[i])) {
					// TODO: show which file failed to be
					// created.
					prompt_show(L"failed to create file!");
					continue;
				}
			}
			
			size_t wname_len = strlen(argv[i]) + 1;
			wchar_t *wname = malloc(sizeof(wchar_t) * wname_len);
			mbstowcs(wname, argv[i], wname_len);

			struct buf b = buf_from_file(argv[i]);
			struct frame f = frame_create(wname, add_buf(&b));
			add_frame(&f);

			free(wname);
		}
	}

	if (frames.size == 0) {
		struct buf b = buf_create(true);
		struct frame f = frame_create(CONF_SCRAP_NAME, add_buf(&b));
		add_frame(&f);
	}

	struct sigaction sa;
	sigaction(SIGWINCH, NULL, &sa);
	old_sigwinch_handler = sa.sa_handler;
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);

	reset_binds();
	set_global_mode();
	arrange_frames();
	editor_redraw();

	return 0;
}

void
editor_main_loop(void)
{
	running = true;

	while (running) {
		// ensure frames sharing buffers are in a valid state.
		for (size_t i = 0; i < frames.size; ++i) {
			struct frame *f = &frames.data[i];
			f->csr = MIN(f->csr, f->buf->size);
		}
		
		mode_update();
		
		editor_redraw();
		
		wint_t k = keybd_await_key();
		if (k != KEYBD_IGNORE && (k == L'\n' || k == L'\t' || iswprint(k))) {
			struct frame *f = &frames.data[cur_frame];
			
			buf_write_wch(f->buf, f->csr, k);
			frame_mv_csr_rel(f, 0, !!(f->buf->flags & BF_WRITABLE), true);
			mode_key_update(k);
		}
	}
}

void
editor_quit(void)
{
	if (clipbuf)
		free(clipbuf);
	
	for (size_t i = 0; i < frames.size; ++i)
		frame_destroy(&frames.data[i]);

	for (size_t i = 0; i < p_bufs.size; ++i) {
		buf_destroy(p_bufs.data[i]);
		free(p_bufs.data[i]);
	}

	vec_frame_destroy(&frames);
	vec_p_buf_destroy(&p_bufs);

	keybd_quit();
}

void
editor_redraw(void)
{
	if (mono) {
		frame_comp_boundary(&frames.data[cur_frame]);
		frame_draw(&frames.data[cur_frame], FDF_ACTIVE | FDF_MONO);
	} else {
		for (size_t i = 0; i < frames.size; ++i) {
			frame_comp_boundary(&frames.data[i]);
			frame_draw(&frames.data[i], FDF_ACTIVE * (i == cur_frame));
		}
	}
	
	// draw current bind status if necessary.
	size_t len;
	if (!keybd_is_exec_mac() && keybd_cur_bind(NULL)
	    || keybd_is_rec_mac() && keybd_cur_mac(NULL)) {
		int const *src = keybd_is_rec_mac() ? keybd_cur_mac(&len) : keybd_cur_bind(&len);
		
		wchar_t dpy[(KEYBD_MAX_DPY_LEN + 1) * KEYBD_MAX_MAC_LEN];
		keybd_key_dpy(dpy, src, len);
		
		struct winsize ws;
		ioctl(0, TIOCGWINSZ, &ws);
		
		size_t draw_start = 0, draw_len = wcslen(dpy);
		while (draw_len > ws.ws_col) {
			wchar_t const *next = wcschr(dpy + draw_start, L' ') + 1;
			if (!next)
				break;
			
			size_t diff = (uintptr_t)next - (uintptr_t)(dpy + draw_start);
			size_t ndiff = diff / sizeof(wchar_t);
			
			draw_start += ndiff;
			draw_len -= ndiff;
		}
		
		draw_put_wstr(ws.ws_row - 1, 0, dpy + draw_start);
		draw_put_attr(ws.ws_row - 1, 0, CONF_A_GHIGH_FG, CONF_A_GHIGH_BG, draw_len);
	}
	
	draw_refresh();
}

static struct buf *
add_buf(struct buf *b)
{
	if (b->src_type == BST_FILE) {
		for (size_t i = 0; i < p_bufs.size; ++i) {
			if (p_bufs.data[i]->src_type == BST_FILE
			    && !strcmp(b->src, p_bufs.data[i]->src)) {
				buf_destroy(b);
				return p_bufs.data[i];
			}
		}
	}

	struct buf *pb = malloc(sizeof(struct buf));
	*pb = *b;
	vec_p_buf_add(&p_bufs, &pb);
	
	return p_bufs.data[p_bufs.size - 1];
}

static struct frame *
add_frame(struct frame *f)
{
	vec_frame_add(&frames, f);
	return &frames.data[frames.size - 1];
}

static void
arrange_frames(void)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	struct frame *f;
	
	if (mono) {
		f = &frames.data[cur_frame];
		f->pr = f->pc = 0;
		f->sr = ws.ws_row;
		f->sc = ws.ws_col;
		return;
	}
	
	f = &frames.data[0];
	f->pr = f->pc = 0;
	f->sr = ws.ws_row;
	f->sc = frames.size == 1 ? ws.ws_col : CONF_MNUM * ws.ws_col / CONF_MDENOM;

	for (size_t i = 1; i < frames.size; ++i) {
		f = &frames.data[i];

		f->sr = ws.ws_row / (frames.size - 1);
		f->pr = (i - 1) * f->sr;
		f->pc = CONF_MNUM * ws.ws_col / CONF_MDENOM;
		f->sc = ws.ws_col - f->pc;

		if (f->pr + f->sr > ws.ws_row || i == frames.size - 1)
			f->sr = ws.ws_row - f->pr;
	}
}

static void
bind_quit(void)
{
	bool mod_exists = false;
	for (size_t i = 0; i < p_bufs.size; ++i) {
		if ((*p_bufs.data[i]).flags & BF_MODIFIED) {
			mod_exists = true;
			break;
		}
	}
	
	if (mod_exists) {
		int confirm = prompt_yes_no(L"there are unsaved modified buffers! quit anyway?", false);
		editor_redraw();
		if (confirm != 1)
			return;
	}
	
	running = false;
}

static void
bind_chg_frame_fwd(void)
{
	cur_frame = (cur_frame + 1) % frames.size;
	reset_binds();
	set_global_mode();
	if (mono)
		arrange_frames();
	editor_redraw();
}

static void
bind_chg_frame_back(void)
{
	cur_frame = (cur_frame == 0 ? frames.size : cur_frame) - 1;
	reset_binds();
	set_global_mode();
	if (mono)
		arrange_frames();
	editor_redraw();
}

static void
bind_focus_frame(void)
{
	if (cur_frame != 0) {
		vec_frame_swap(&frames, 0, cur_frame);
		cur_frame = 0;
		reset_binds();
		set_global_mode();
		arrange_frames();
		editor_redraw();
	}
}

static void
bind_kill_frame(void)
{
	int confirm = prompt_yes_no(L"kill active frame?", false);
	editor_redraw();
	if (confirm != 1)
		return;
	
	frame_destroy(&frames.data[cur_frame]);
	vec_frame_rm(&frames, cur_frame);
	cur_frame = cur_frame > 0 ? cur_frame - 1 : 0;

	// destroy orphaned buffers.
	for (size_t i = 0; i < p_bufs.size; ++i) {
		bool orphan = true;
		for (size_t j = 0; j < frames.size; ++j) {
			if (p_bufs.data[i] == frames.data[j].buf) {
				orphan = false;
				break;
			}
		}

		if (orphan) {
			buf_destroy(p_bufs.data[i]);
			free(p_bufs.data[i]);
			vec_p_buf_rm(&p_bufs, i--);
		}
	}

	if (frames.size == 0) {
		struct buf b = buf_create(true);
		struct frame f = frame_create(CONF_SCRAP_NAME, add_buf(&b));
		add_frame(&f);
	}

	reset_binds();
	set_global_mode();
	arrange_frames();
	editor_redraw();
}

static void
bind_open_file(void)
{
	wchar_t *wpath = prompt_ask(L"open file: ", prompt_comp_path, NULL);
	editor_redraw();
	if (!wpath)
		return;

	size_t path_size = sizeof(wchar_t) * (wcslen(wpath) + 1);
	char *path = malloc(path_size);
	wcstombs(path, wpath, path_size);

	struct stat s;
	if (stat(path, &s) || !S_ISREG(s.st_mode)) {
		prompt_show(L"could not open file!");
		editor_redraw();
		free(path);
		return;
	}

	struct buf buf = buf_from_file(path);
	struct frame frame = frame_create(wpath, add_buf(&buf));
	add_frame(&frame);

	cur_frame = frames.size - 1;
	reset_binds();
	set_global_mode();
	arrange_frames();
	editor_redraw();

	free(path);
	free(wpath);
}

static void
bind_save_file(void)
{
	struct frame *f = &frames.data[cur_frame];
	enum buf_src_type prev_type = f->buf->src_type;
	uint8_t prev_flags = f->buf->flags;

	wchar_t *w_path;
	if (f->buf->src_type == BST_FRESH) {
		f->buf->src_type = BST_FILE;
		
		w_path = prompt_ask(L"save to file: ", prompt_comp_path, NULL);
		editor_redraw();

		if (w_path) {
			size_t path_size = sizeof(wchar_t) * (wcslen(w_path) + 1);
			char *path = malloc(path_size);
			wcstombs(path, w_path, path_size);

			f->buf->src = strdup(path);
			free(path);

			// force resave and allow editing of newly made file.
			f->buf->flags |= BF_MODIFIED | BF_WRITABLE;
		} else
			f->buf->src = NULL;
	}

	// no path was given as source for new buffer, so the buffer state is
	// reset to how it was before the save.
	if (f->buf->src && !*(uint8_t *)f->buf->src) {
		prompt_show(L"no save path given, nothing will be done!");
		editor_redraw();
	}
	
	if (!f->buf->src || !*(uint8_t *)f->buf->src) {
		f->buf->src_type = prev_type;
		f->buf->flags = prev_flags;
		
		if (w_path)
			free(w_path);
		
		return;
	}
	
	if (prev_type == BST_FRESH)
		mk_file(f->buf->src);

	if (buf_save(f->buf)) {
		prompt_show(L"failed to write file!");
		editor_redraw();
		free(w_path);
		return;
	}

	if (prev_type == BST_FRESH) {
		free(f->name);
		f->name = w_path;
	}

	if (!f->local_mode[0]) {
		free(f->local_mode);
		f->local_mode = strdup(file_ext(f->buf->src));
	}

	if (!mode_get())
		set_global_mode();
}

static void
bind_nav_fwd_ch(void)
{
	frame_mv_csr_rel(&frames.data[cur_frame], 0, 1, true);
}

static void
bind_nav_fwd_word(void)
{
	struct frame *f = &frames.data[cur_frame];

	while (f->csr < f->buf->size && !iswalnum(f->buf->conts[f->csr]))
		++f->csr;
	while (f->csr < f->buf->size && iswalnum(f->buf->conts[f->csr]))
		++f->csr;
	
	++f->csr;
	frame_mv_csr_rel(f, 0, -1, true);
}

static void
bind_nav_fwd_page(void)
{
	struct frame *f = &frames.data[cur_frame];

	f->csr = f->buf_start;
	frame_mv_csr_rel(f, f->sr - 1, 0, false);
	
	f->buf_start = f->csr;
	while (f->buf_start > 0 && f->buf->conts[f->buf_start - 1] != L'\n')
		--f->buf_start;

	frame_comp_boundary(f);
}

static void
bind_nav_back_ch(void)
{
	frame_mv_csr_rel(&frames.data[cur_frame], 0, -1, true);
}

static void
bind_nav_back_word(void)
{
	struct frame *f = &frames.data[cur_frame];

	while (f->csr > 0 && !iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;
	while (f->csr > 0 && iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;

	++f->csr;
	frame_mv_csr_rel(f, 0, -1, true);
}

static void
bind_nav_back_page(void)
{
	struct frame *f = &frames.data[cur_frame];
	f->csr = f->buf_start;
	frame_mv_csr_rel(f, 1 - f->sr, 0, false);
}

static void
bind_nav_down(void)
{
	frame_mv_csr_rel(&frames.data[cur_frame], 1, 0, false);
}

static void
bind_nav_up(void)
{
	frame_mv_csr_rel(&frames.data[cur_frame], -1, 0, false);
}

static void
bind_nav_ln_start(void)
{
	frame_mv_csr_rel(&frames.data[cur_frame], 0, -INT_MAX, false);
}

static void
bind_nav_ln_end(void)
{
	frame_mv_csr_rel(&frames.data[cur_frame], 0, INT_MAX, false);
}

static void
bind_nav_goto(void)
{
ask_again:;
	wchar_t *ws_linum = prompt_ask(L"goto line: ", NULL, NULL);
	editor_redraw();
	if (!ws_linum)
		return;

	if (!*ws_linum) {
		free(ws_linum);
		prompt_show(L"expected a line number!");
		editor_redraw();
		goto ask_again;
	}

	size_t s_linum_size = sizeof(wchar_t) * (wcslen(ws_linum) + 1);
	char *s_linum = malloc(s_linum_size);
	wcstombs(s_linum, ws_linum, s_linum_size);
	free(ws_linum);

	for (char const *c = s_linum; *c; ++c) {
		if (!isdigit(*c)) {
			free(s_linum);
			prompt_show(L"invalid line number!");
			editor_redraw();
			goto ask_again;
		}
	}

	unsigned linum = atoi(s_linum);
	free(s_linum);
	linum -= linum != 0;

	frame_mv_csr(&frames.data[cur_frame], linum, 0);
}

static void
bind_del_fwd_ch(void)
{
	struct frame *f = &frames.data[cur_frame];
	if (f->csr < f->buf->size)
		buf_erase(f->buf, f->csr, f->csr + 1);
}

static void
bind_del_back_ch(void)
{
	struct frame *f = &frames.data[cur_frame];

	if (f->csr > 0 && f->buf->flags & BF_WRITABLE) {
		frame_mv_csr_rel(f, 0, -1, true);
		buf_erase(f->buf, f->csr, f->csr + 1);
		frame_comp_boundary(f);
	}
}

static void
bind_del_back_word(void)
{
	struct frame *f = &frames.data[cur_frame];
	if (!(f->buf->flags & BF_WRITABLE))
		return;

	size_t ub = f->csr;

	while (f->csr > 0 && !iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;
	while (f->csr > 0 && iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;

	++f->csr;
	frame_mv_csr_rel(f, 0, -1, true);
	buf_erase(f->buf, f->csr, ub);
}

static void
bind_chg_mode_global(void)
{
	wchar_t *w_new_gm = prompt_ask(L"new globalmode: ", NULL, NULL);
	editor_redraw();
	if (!w_new_gm)
		return;

	size_t new_gm_size = sizeof(wchar_t) * (wcslen(w_new_gm) + 1);
	char *new_gm = malloc(new_gm_size);
	wcstombs(new_gm, w_new_gm, new_gm_size);
	free(w_new_gm);

	reset_binds();
	mode_set(new_gm, &frames.data[cur_frame]);
	
	free(new_gm);
}

static void
bind_chg_mode_local(void)
{
	wchar_t *w_new_lm = prompt_ask(L"new frame localmode: ", NULL, NULL);
	editor_redraw();
	if (!w_new_lm)
		return;

	size_t new_lm_size = sizeof(wchar_t) * (wcslen(w_new_lm) + 1);
	char *new_lm = malloc(new_lm_size);
	wcstombs(new_lm, w_new_lm, new_lm_size);
	free(w_new_lm);

	free(frames.data[cur_frame].local_mode);
	frames.data[cur_frame].local_mode = strdup(new_lm);
	
	free(new_lm);
}

static void
bind_create_scrap(void)
{
	struct buf b = buf_create(true);
	struct frame f = frame_create(CONF_SCRAP_NAME, add_buf(&b));
	add_frame(&f);
	
	cur_frame = frames.size - 1;
	reset_binds();
	set_global_mode();
	arrange_frames();
	editor_redraw();
}

static void
bind_new_line(void)
{
	struct frame *f = &frames.data[cur_frame];
	buf_push_hist_brk(f->buf);
	buf_write_wch(f->buf, f->csr, L'\n');
	frame_mv_csr_rel(f, 0, !!(f->buf->flags & BF_WRITABLE), true);
}

static void
bind_focus(void)
{
	struct frame *f = &frames.data[cur_frame];
	
	unsigned csrr, csrc;
	buf_pos(f->buf, f->csr, &csrr, &csrc);

	f->buf_start = 0;
	unsigned bsr = 0;
	long dst_bsr = (long)csrr - (f->sr - 1) / 2;
	dst_bsr = MAX(dst_bsr, 0);
	while (f->buf_start < f->buf->size && bsr < dst_bsr) {
		if (f->buf->conts[f->buf_start++] == L'\n')
			++bsr;
	}

	frame_comp_boundary(f);
}

static void
bind_kill(void)
{
	struct frame *f = &frames.data[cur_frame];
	if (!(f->buf->flags & BF_WRITABLE))
		return;
	
	if (clipbuf)
		free(clipbuf);
	
	size_t ln_end = f->csr;
	while (ln_end < f->buf->size && f->buf->conts[ln_end] != L'\n')
		++ln_end;
	
	size_t cp_size = (ln_end - f->csr) * sizeof(wchar_t);
	clipbuf = malloc(cp_size + sizeof(wchar_t));
	clipbuf[ln_end - f->csr] = 0;
	memcpy(clipbuf, f->buf->conts + f->csr, cp_size);
	
	buf_erase(f->buf, f->csr, ln_end);
}

static void
bind_paste(void)
{
	struct frame *f = &frames.data[cur_frame];
	if (!(f->buf->flags & BF_WRITABLE))
		return;
	
	if (!clipbuf) {
		prompt_show(L"clipbuffer is empty!");
		editor_redraw();
		return;
	}
	
	buf_write_wstr(f->buf, f->csr, clipbuf);
	f->csr += wcslen(clipbuf);
	frame_comp_boundary(f);
}

static void
bind_undo(void)
{
	struct frame *f = &frames.data[cur_frame];

	if (f->buf->hist.size == 0) {
		prompt_show(L"no further undo information!");
		editor_redraw();
		return;
	}

	struct buf_op const *bo = &f->buf->hist.data[f->buf->hist.size - 1];
	while (bo > f->buf->hist.data && bo->type == BOT_BRK)
		--bo;
	
	size_t csr_dst;
	switch (bo->type) {
	case BOT_WRITE:
		csr_dst = bo->lb;
		break;
	case BOT_ERASE:
		csr_dst = bo->ub;
		break;
	}

	if (buf_undo(f->buf)) {
		prompt_show(L"failed to undo last operation!");
		editor_redraw();
		return;
	}

	unsigned csrr, csrc;
	buf_pos(f->buf, csr_dst, &csrr, &csrc);
	frame_mv_csr(f, csrr, csrc);
	f->csr_want_col = csrc;
}

static void
bind_copy(void)
{
	if (clipbuf)
		free(clipbuf);
	
	struct frame *f = &frames.data[cur_frame];
	
	size_t ln = f->csr;
	while (ln > 0 && f->buf->conts[ln - 1] != L'\n')
		--ln;
	
	size_t ln_end = f->csr;
	while (ln_end < f->buf->size && f->buf->conts[ln_end] != L'\n')
		++ln_end;
	
	size_t cp_size = (ln_end - ln) * sizeof(wchar_t);
	clipbuf = malloc(cp_size + sizeof(wchar_t));
	clipbuf[ln_end - ln] = 0;
	memcpy(clipbuf, f->buf->conts + ln, cp_size);
}

static void
bind_ncopy(void)
{
ask_again:;
	wchar_t *ws_ln_cnt = prompt_ask(L"copy lines: ", NULL, NULL);
	editor_redraw();
	if (!ws_ln_cnt)
		return;
	
	if (!*ws_ln_cnt) {
		free(ws_ln_cnt);
		prompt_show(L"expected a line count!");
		editor_redraw();
		goto ask_again;
	}
	
	size_t s_ln_cnt_size = sizeof(wchar_t) * (wcslen(ws_ln_cnt) + 1);
	char *s_ln_cnt = malloc(s_ln_cnt_size);
	wcstombs(s_ln_cnt, ws_ln_cnt, s_ln_cnt_size);
	free(ws_ln_cnt);
	
	for (char const *c = s_ln_cnt; *c; ++c) {
		if (!isdigit(*c)) {
			free(s_ln_cnt);
			prompt_show(L"invalid line count!");
			editor_redraw();
			goto ask_again;
		}
	}
	
	unsigned ln_cnt = atoi(s_ln_cnt);
	free(s_ln_cnt);
	if (!ln_cnt) {
		prompt_show(L"cannot copy zero lines!");
		editor_redraw();
		goto ask_again;
	}
	
	if (clipbuf)
		free(clipbuf);
	
	struct frame *f = &frames.data[cur_frame];
	
	size_t reg_lb = f->csr;
	while (reg_lb > 0 && f->buf->conts[reg_lb - 1] != L'\n')
		--reg_lb;
	
	size_t reg_ub = f->csr;
	while (reg_ub < f->buf->size && ln_cnt > 0) {
		while (f->buf->conts[reg_ub] != L'\n')
			++reg_ub;
		--ln_cnt;
		reg_ub += ln_cnt > 0;
	}
	
	size_t cp_size = (reg_ub - reg_lb) * sizeof(wchar_t);
	clipbuf = malloc(cp_size + sizeof(wchar_t));
	clipbuf[reg_ub - reg_lb] = 0;
	memcpy(clipbuf, f->buf->conts + reg_lb, cp_size);
}

static void
bind_find_lit(void)
{
ask_again:;
	wchar_t *needle = prompt_ask(L"find literally: ", NULL, NULL);
	editor_redraw();
	if (!needle)
		return;
	
	if (!*needle) {
		free(needle);
		prompt_show(L"expected a needle!");
		editor_redraw();
		goto ask_again;
	}
	
	size_t search_pos, needle_len = wcslen(needle);
	struct frame *f = &frames.data[cur_frame];
	for (search_pos = f->csr + 1; search_pos < f->buf->size; ++search_pos) {
		if (!wcsncmp(&f->buf->conts[search_pos], needle, needle_len))
			break;
	}
	
	free(needle);
	
	if (search_pos == f->buf->size) {
		prompt_show(L"did not find needle in haystack!");
		editor_redraw();
		return;
	}
	
	unsigned r, c;
	buf_pos(f->buf, search_pos, &r, &c);
	frame_mv_csr(f, r, c);
}

static void
bind_mac_begin(void)
{
	keybd_rec_mac_begin();
}

static void
bind_mac_end(void)
{
	if (keybd_is_rec_mac())
		keybd_rec_mac_end();
	else {
		keybd_rec_mac_end();
		
		if (!keybd_cur_mac(NULL)) {
			prompt_show(L"no macro specified to execute!");
			editor_redraw();
			return;
		}
		
		keybd_exec_mac();
	}
}

static void
bind_toggle_mono(void)
{
	mono = !mono;
	arrange_frames();
	editor_redraw();
}

static void
reset_binds(void)
{
	// quit and reinit to reset current keybind buffer and bind information.
	keybd_quit();
	keybd_init();

	keybd_bind(conf_bind_quit, bind_quit);
	keybd_bind(conf_bind_chg_frame_fwd, bind_chg_frame_fwd);
	keybd_bind(conf_bind_chg_frame_back, bind_chg_frame_back);
	keybd_bind(conf_bind_focus_frame, bind_focus_frame);
	keybd_bind(conf_bind_kill_frame, bind_kill_frame);
	keybd_bind(conf_bind_open_file, bind_open_file);
	keybd_bind(conf_bind_save_file, bind_save_file);
	keybd_bind(conf_bind_nav_fwd_ch, bind_nav_fwd_ch);
	keybd_bind(conf_bind_nav_fwd_word, bind_nav_fwd_word);
	keybd_bind(conf_bind_nav_fwd_page, bind_nav_fwd_page);
	keybd_bind(conf_bind_nav_back_ch, bind_nav_back_ch);
	keybd_bind(conf_bind_nav_back_word, bind_nav_back_word);
	keybd_bind(conf_bind_nav_back_page, bind_nav_back_page);
	keybd_bind(conf_bind_nav_down, bind_nav_down);
	keybd_bind(conf_bind_nav_up, bind_nav_up);
	keybd_bind(conf_bind_nav_ln_start, bind_nav_ln_start);
	keybd_bind(conf_bind_nav_ln_end, bind_nav_ln_end);
	keybd_bind(conf_bind_nav_goto, bind_nav_goto);
	keybd_bind(conf_bind_del_fwd_ch, bind_del_fwd_ch);
	keybd_bind(conf_bind_del_back_ch, bind_del_back_ch);
	keybd_bind(conf_bind_del_back_word, bind_del_back_word);
	keybd_bind(conf_bind_chg_mode_global, bind_chg_mode_global);
	keybd_bind(conf_bind_chg_mode_local, bind_chg_mode_local);
	keybd_bind(conf_bind_create_scrap, bind_create_scrap);
	keybd_bind(conf_bind_new_line, bind_new_line);
	keybd_bind(conf_bind_focus, bind_focus);
	keybd_bind(conf_bind_kill, bind_kill);
	keybd_bind(conf_bind_paste, bind_paste);
	keybd_bind(conf_bind_undo, bind_undo);
	keybd_bind(conf_bind_copy, bind_copy);
	keybd_bind(conf_bind_ncopy, bind_ncopy);
	keybd_bind(conf_bind_find_lit, bind_find_lit);
	keybd_bind(conf_bind_mac_begin, bind_mac_begin);
	keybd_bind(conf_bind_mac_end, bind_mac_end);
	keybd_bind(conf_bind_toggle_mono, bind_toggle_mono);
	
	keybd_organize();
}

static void
set_global_mode(void)
{
	struct frame *f = &frames.data[cur_frame];
	if (f->buf->src_type != BST_FILE)
		return;

	char const *buf_ext = file_ext(f->buf->src);
	for (size_t i = 0; i < conf_metab_size; ++i) {
		for (char const **ext = conf_metab[i].exts; *ext; ++ext) {
			if (!strcmp(buf_ext, *ext)) {
				mode_set(conf_metab[i].mode, f);
				return;
			}
		}
	}

	mode_set(NULL, f);
}

static void
sigwinch_handler(int arg)
{
	old_sigwinch_handler(arg);
	arrange_frames();
	editor_redraw();
}
