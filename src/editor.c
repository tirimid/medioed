#include "editor.h"

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include <sys/stat.h>

#include "buf.h"
#include "conf.h"
#include "draw.h"
#include "editor_bind.h"
#include "frame.h"
#include "keybd.h"
#include "prompt.h"

// you may want to change the value of this, but you'll need to do some testing
// and play around with values to find something good.
// check `sigwinch_handler()` for more information.
#define WIN_RESIZE_SPIN 10000000

extern bool flag_c;

// global editor state.
// access carefully, preferably *only* in `src/editor_bind.c`.
bool editor_running;
size_t editor_cur_frame;
struct vec_frame editor_frames;
struct vec_p_buf editor_p_bufs;
wchar_t *editor_clipbuf = NULL;
bool editor_mono = false;
bool editor_ignore_sigwinch = false; // used for man reader hack.

static void open_arg_files(int argc, int first_arg, char const *argv[]);
static void sigwinch_handler(int arg);

static void (*old_sigwinch_handler)(int);

int
editor_init(int argc, char const *argv[])
{
	keybd_init();
	
	editor_frames = vec_frame_create();
	editor_p_bufs = vec_p_buf_create();
	editor_cur_frame = 0;
	
	int first_arg = 1;
	while (first_arg < argc && *argv[first_arg] == '-')
		++first_arg;

	if (argc <= first_arg)
	{
		struct buf gb = buf_from_wstr(CONF_GREET_TEXT, false);
		struct frame gf = frame_create(CONF_GREET_NAME, editor_add_buf(&gb));
		editor_add_frame(&gf);
	}
	else
		open_arg_files(argc, first_arg, argv);

	if (editor_frames.size == 0)
	{
		struct buf b = buf_create(true);
		struct frame f = frame_create(CONF_SCRAP_NAME, editor_add_buf(&b));
		editor_add_frame(&f);
	}

	struct sigaction sa;
	sigaction(SIGWINCH, NULL, &sa);
	old_sigwinch_handler = sa.sa_handler;
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);

	editor_reset_binds();
	editor_set_global_mode();
	editor_arrange_frames();
	editor_redraw();

	return 0;
}

void
editor_main_loop(void)
{
	editor_running = true;

	while (editor_running)
	{
		// ensure frames sharing buffers are in a valid state.
		for (size_t i = 0; i < editor_frames.size; ++i)
		{
			struct frame *f = &editor_frames.data[i];
			f->csr = MIN(f->csr, f->buf->size);
		}
		
		mode_update();
		
		editor_redraw();
		
		wint_t k = keybd_await_key();
		if (k != KEYBD_IGNORE && (wcschr(L"\n\t", k) || iswprint(k)))
		{
			struct frame *f = &editor_frames.data[editor_cur_frame];
			
			buf_write_wch(f->buf, f->csr, k);
			frame_mv_csr_rel(f, 0, !!(f->buf->flags & BF_WRITABLE), true);
			mode_key_update(k);
		}
	}
}

void
editor_quit(void)
{
	if (editor_clipbuf)
		free(editor_clipbuf);
	
	for (size_t i = 0; i < editor_frames.size; ++i)
		frame_destroy(&editor_frames.data[i]);

	for (size_t i = 0; i < editor_p_bufs.size; ++i)
	{
		buf_destroy(editor_p_bufs.data[i]);
		free(editor_p_bufs.data[i]);
	}
	
	vec_frame_destroy(&editor_frames);
	vec_p_buf_destroy(&editor_p_bufs);
	
	keybd_quit();
}

void
editor_redraw(void)
{
	if (editor_mono)
	{
		struct frame *f = &editor_frames.data[editor_cur_frame];
		frame_comp_boundary(f);
		frame_draw(f, FDF_ACTIVE | FDF_MONO);
	}
	else
	{
		for (size_t i = 0; i < editor_frames.size; ++i)
		{
			frame_comp_boundary(&editor_frames.data[i]);
			frame_draw(&editor_frames.data[i], FDF_ACTIVE * (i == editor_cur_frame));
		}
	}
	
	// draw current bind status if necessary.
	size_t len;
	if (!keybd_is_exec_mac() && keybd_cur_bind(NULL)
	    || keybd_is_rec_mac() && keybd_cur_mac(NULL))
	{
		int const *src = keybd_is_rec_mac() ? keybd_cur_mac(&len) : keybd_cur_bind(&len);
		
		wchar_t dpy[(KEYBD_MAX_DPY_LEN + 1) * KEYBD_MAX_MAC_LEN];
		keybd_key_dpy(dpy, src, len);
		
		struct win_size ws = draw_win_size();
		size_t draw_start = 0, draw_len = wcslen(dpy);
		while (draw_len > ws.sc)
		{
			wchar_t const *next = wcschr(dpy + draw_start, L' ') + 1;
			if (!next)
				break;
			
			size_t diff = (uintptr_t)next - (uintptr_t)(dpy + draw_start);
			size_t ndiff = diff / sizeof(wchar_t);
			
			draw_start += ndiff;
			draw_len -= ndiff;
		}
		
		draw_put_wstr(ws.sr - 1, 0, dpy + draw_start);
		draw_put_attr(ws.sr - 1, 0, CONF_A_GHIGH_FG, CONF_A_GHIGH_BG, draw_len);
	}
	
	draw_refresh();
}

struct buf *
editor_add_buf(struct buf *b)
{
	if (b->src_type == BST_FILE)
	{
		for (size_t i = 0; i < editor_p_bufs.size; ++i)
		{
			if (editor_p_bufs.data[i]->src_type != BST_FILE)
				continue;
			
			if (is_path_same(b->src, editor_p_bufs.data[i]->src))
			{
				buf_destroy(b);
				return editor_p_bufs.data[i];
			}
		}
	}

	struct buf *pb = malloc(sizeof(struct buf));
	*pb = *b;
	vec_p_buf_add(&editor_p_bufs, &pb);
	
	return editor_p_bufs.data[editor_p_bufs.size - 1];
}

struct frame *
editor_add_frame(struct frame *f)
{
	for (size_t i = 0; i < editor_frames.size; ++i)
	{
		if (editor_frames.data[i].buf == f->buf)
		{
			prompt_show(L"cannot assign multiple frames to one buffer!");
			editor_redraw();
			frame_destroy(f);
			return &editor_frames.data[i];
		}
	}
	
	vec_frame_add(&editor_frames, f);
	
	return &editor_frames.data[editor_frames.size - 1];
}

void
editor_arrange_frames(void)
{
	struct win_size ws = draw_win_size();

	struct frame *f;
	
	if (editor_mono)
	{
		f = &editor_frames.data[editor_cur_frame];
		f->pr = f->pc = 0;
		f->sr = ws.sr;
		f->sc = ws.sc;
		return;
	}
	
	f = &editor_frames.data[0];
	f->pr = f->pc = 0;
	f->sr = ws.sr;
	f->sc = editor_frames.size == 1 ? ws.sc : CONF_MNUM * ws.sc / CONF_MDENOM;

	for (size_t i = 1; i < editor_frames.size; ++i)
	{
		f = &editor_frames.data[i];

		f->sr = ws.sr / (editor_frames.size - 1);
		f->pr = (i - 1) * f->sr;
		f->pc = CONF_MNUM * ws.sc / CONF_MDENOM;
		f->sc = ws.sc - f->pc;

		if (f->pr + f->sr > ws.sr || i == editor_frames.size - 1)
			f->sr = ws.sr - f->pr;
	}
}

void
editor_reset_binds(void)
{
	// quit and reinit to reset current keybind buffer and bind information.
	keybd_quit();
	keybd_init();

	keybd_bind(conf_bind_quit, editor_bind_quit);
	keybd_bind(conf_bind_chg_frame_fwd, editor_bind_chg_frame_fwd);
	keybd_bind(conf_bind_chg_frame_back, editor_bind_chg_frame_back);
	keybd_bind(conf_bind_focus_frame, editor_bind_focus_frame);
	keybd_bind(conf_bind_kill_frame, editor_bind_kill_frame);
	keybd_bind(conf_bind_open_file, editor_bind_open_file);
	keybd_bind(conf_bind_save_file, editor_bind_save_file);
	keybd_bind(conf_bind_nav_fwd_ch, editor_bind_nav_fwd_ch);
	keybd_bind(conf_bind_nav_fwd_word, editor_bind_nav_fwd_word);
	keybd_bind(conf_bind_nav_fwd_page, editor_bind_nav_fwd_page);
	keybd_bind(conf_bind_nav_back_ch, editor_bind_nav_back_ch);
	keybd_bind(conf_bind_nav_back_word, editor_bind_nav_back_word);
	keybd_bind(conf_bind_nav_back_page, editor_bind_nav_back_page);
	keybd_bind(conf_bind_nav_down, editor_bind_nav_down);
	keybd_bind(conf_bind_nav_up, editor_bind_nav_up);
	keybd_bind(conf_bind_nav_ln_start, editor_bind_nav_ln_start);
	keybd_bind(conf_bind_nav_ln_end, editor_bind_nav_ln_end);
	keybd_bind(conf_bind_nav_goto, editor_bind_nav_goto);
	keybd_bind(conf_bind_del_fwd_ch, editor_bind_del_fwd_ch);
	keybd_bind(conf_bind_del_back_ch, editor_bind_del_back_ch);
	keybd_bind(conf_bind_del_back_word, editor_bind_del_back_word);
	keybd_bind(conf_bind_chg_mode_global, editor_bind_chg_mode_global);
	keybd_bind(conf_bind_chg_mode_local, editor_bind_chg_mode_local);
	keybd_bind(conf_bind_create_scrap, editor_bind_create_scrap);
	keybd_bind(conf_bind_new_line, editor_bind_new_line);
	keybd_bind(conf_bind_focus, editor_bind_focus);
	keybd_bind(conf_bind_kill, editor_bind_kill);
	keybd_bind(conf_bind_paste, editor_bind_paste);
	keybd_bind(conf_bind_undo, editor_bind_undo);
	keybd_bind(conf_bind_copy, editor_bind_copy);
	keybd_bind(conf_bind_ncopy, editor_bind_ncopy);
	keybd_bind(conf_bind_find_lit, editor_bind_find_lit);
	keybd_bind(conf_bind_mac_begin, editor_bind_mac_begin);
	keybd_bind(conf_bind_mac_end, editor_bind_mac_end);
	keybd_bind(conf_bind_toggle_mono, editor_bind_toggle_mono);
	keybd_bind(conf_bind_read_man_word, editor_bind_read_man_word);
	keybd_bind(conf_bind_file_exp, editor_bind_file_exp);
	
	keybd_organize();
}

void
editor_set_global_mode(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	if (f->buf->src_type != BST_FILE)
		return;

	char const *buf_ext = file_ext(f->buf->src);
	for (size_t i = 0; i < conf_metab_size; ++i)
	{
		for (char const **ext = conf_metab[i].exts; *ext; ++ext)
		{
			if (!strcmp(buf_ext, *ext))
			{
				mode_set(conf_metab[i].globalmode, f);
				return;
			}
		}
	}

	mode_set(NULL, f);
}

static void
open_arg_files(int argc, int first_arg, char const *argv[])
{
	for (int i = first_arg; i < argc; ++i)
	{
		draw_clear(L' ', CONF_A_GNORM_FG, CONF_A_GNORM_BG);
		
		struct stat s;
		if ((stat(argv[i], &s) || !S_ISREG(s.st_mode)) && !flag_c)
		{
			size_t wmsg_len = strlen(argv[i]) + 22;
			wchar_t *wmsg = malloc(sizeof(wchar_t) * wmsg_len);
			mbstowcs(wmsg + 21, argv[i], wmsg_len);
			wcsncpy(wmsg, L"failed to open file: ", 21);
			
			prompt_show(wmsg);
			free(wmsg);
			
			continue;
		}
		else if (stat(argv[i], &s) && flag_c)
		{
			if (mk_file(argv[i]))
			{
				size_t wmsg_len = strlen(argv[i]) + 24;
				wchar_t *wmsg = malloc(sizeof(wchar_t) * wmsg_len);
				mbstowcs(wmsg + 23, argv[i], wmsg_len);
				wcsncpy(wmsg, L"failed to create file: ", 23);
				
				prompt_show(wmsg);
				free(wmsg);
				
				continue;
			}
		}
		
		size_t wname_len = strlen(argv[i]) + 1;
		wchar_t *wname = malloc(sizeof(wchar_t) * wname_len);
		mbstowcs(wname, argv[i], wname_len);
		
		struct buf b = buf_from_file(argv[i]);
		struct frame f = frame_create(wname, editor_add_buf(&b));
		editor_add_frame(&f);
		
		free(wname);
	}
}

static void
sigwinch_handler(int arg)
{
	if (editor_ignore_sigwinch)
		return;
	
	// small wait necessary for window size to stabilize.
	// sometimes, when resized, I've noticed that the terminal emulator will
	// switch to a certain window size, then almost instantly switch to
	// something else.
	// spinning for a bit will allow the window to settle on its final size,
	// before doing any kind of rendering or draw system changes, so that the
	// visual output doesn't fuck itself until an update.
	for (int volatile i = 0; i < WIN_RESIZE_SPIN; ++i)
		;
	
	old_sigwinch_handler(arg);
	editor_arrange_frames();
	editor_redraw();
}
