#include "editor_bind.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "conf.h"
#include "editor.h"
#include "file_exp.h"
#include "frame.h"
#include "keybd.h"
#include "label.h"
#include "prompt.h"

extern bool editor_running;
extern size_t editor_cur_frame;
extern struct vec_frame editor_frames;
extern struct vec_p_buf editor_p_bufs;
extern wchar_t *editor_clipbuf;
extern bool editor_mono;
extern bool editor_ignore_sigwinch;

void
editor_bind_quit(void)
{
	bool mod_exists = false;
	for (size_t i = 0; i < editor_p_bufs.size; ++i) {
		if (editor_p_bufs.data[i]->flags & BF_MODIFIED) {
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
	
	editor_running = false;
}

void
editor_bind_chg_frame_fwd(void)
{
	editor_cur_frame = (editor_cur_frame + 1) % editor_frames.size;
	editor_reset_binds();
	editor_set_global_mode();
	if (editor_mono)
		editor_arrange_frames();
	editor_redraw();
}

void
editor_bind_chg_frame_back(void)
{
	if (editor_cur_frame == 0)
		editor_cur_frame = editor_frames.size - 1;
	else
		editor_cur_frame -= 1;
	
	editor_reset_binds();
	editor_set_global_mode();
	if (editor_mono)
		editor_arrange_frames();
	editor_redraw();
}

void
editor_bind_focus_frame(void)
{
	if (editor_cur_frame != 0) {
		vec_frame_swap(&editor_frames, 0, editor_cur_frame);
		editor_cur_frame = 0;
		editor_reset_binds();
		editor_set_global_mode();
		editor_arrange_frames();
		editor_redraw();
	}
}

void
editor_bind_kill_frame(void)
{
	int confirm = prompt_yes_no(L"kill active frame?", false);
	editor_redraw();
	if (confirm != 1)
		return;
	
	frame_destroy(&editor_frames.data[editor_cur_frame]);
	vec_frame_rm(&editor_frames, editor_cur_frame);
	editor_cur_frame = editor_cur_frame > 0 ? editor_cur_frame - 1 : 0;

	// destroy orphaned buffers.
	for (size_t i = 0; i < editor_p_bufs.size; ++i) {
		bool orphan = true;
		for (size_t j = 0; j < editor_frames.size; ++j) {
			if (editor_p_bufs.data[i] == editor_frames.data[j].buf) {
				orphan = false;
				break;
			}
		}

		if (orphan) {
			buf_destroy(editor_p_bufs.data[i]);
			free(editor_p_bufs.data[i]);
			vec_p_buf_rm(&editor_p_bufs, i--);
		}
	}

	if (editor_frames.size == 0) {
		struct buf b = buf_create(true);
		struct frame f = frame_create(CONF_SCRAP_NAME, editor_add_buf(&b));
		editor_add_frame(&f);
	}

	editor_reset_binds();
	editor_set_global_mode();
	editor_arrange_frames();
	editor_redraw();
}

void
editor_bind_open_file(void)
{
	wchar_t *wpath = prompt_ask(L"open file: ", prompt_comp_path);
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
		free(wpath);
		return;
	}
	
	struct buf buf = buf_from_file(path);
	struct frame frame = frame_create(wpath, editor_add_buf(&buf));
	editor_add_frame(&frame);
	
	editor_cur_frame = editor_frames.size - 1;
	editor_reset_binds();
	editor_set_global_mode();
	editor_arrange_frames();
	editor_redraw();
	
	free(path);
	free(wpath);
}

void
editor_bind_save_file(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	enum buf_src_type prev_type = f->buf->src_type;
	uint8_t prev_flags = f->buf->flags;

	wchar_t *w_path;
	if (f->buf->src_type == BST_FRESH) {
		f->buf->src_type = BST_FILE;
		
		w_path = prompt_ask(L"save to file: ", prompt_comp_path);
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
		editor_set_global_mode();
}

void
editor_bind_nav_fwd_ch(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	frame_mv_csr_rel(f, 0, 1, true);
}

void
editor_bind_nav_fwd_word(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];

	while (f->csr < f->buf->size && !iswalnum(buf_get_wch(f->buf, f->csr)))
		++f->csr;
	while (f->csr < f->buf->size && iswalnum(buf_get_wch(f->buf, f->csr)))
		++f->csr;
	
	++f->csr;
	frame_mv_csr_rel(f, 0, -1, true);
}

void
editor_bind_nav_fwd_page(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];

	f->csr = f->buf_start;
	frame_mv_csr_rel(f, f->sr - 1, 0, false);
	
	f->buf_start = f->csr;
	while (f->buf_start > 0
	       && buf_get_wch(f->buf, f->buf_start - 1) != L'\n') {
		--f->buf_start;
	}

	frame_comp_boundary(f);
}

void
editor_bind_nav_back_ch(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	frame_mv_csr_rel(f, 0, -1, true);
}

void
editor_bind_nav_back_word(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];

	while (f->csr > 0 && !iswalnum(buf_get_wch(f->buf, f->csr - 1)))
		--f->csr;
	while (f->csr > 0 && iswalnum(buf_get_wch(f->buf, f->csr - 1)))
		--f->csr;
	
	++f->csr;
	frame_mv_csr_rel(f, 0, -1, true);
}

void
editor_bind_nav_back_page(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	f->csr = f->buf_start;
	frame_mv_csr_rel(f, 1 - f->sr, 0, false);
}

void
editor_bind_nav_down(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	frame_mv_csr_rel(f, 1, 0, false);
}

void
editor_bind_nav_up(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	frame_mv_csr_rel(f, -1, 0, false);
}

void
editor_bind_nav_ln_start(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	frame_mv_csr_rel(f, 0, -INT_MAX, false);
}

void
editor_bind_nav_ln_end(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	frame_mv_csr_rel(f, 0, INT_MAX, false);
}

void
editor_bind_nav_goto(void)
{
ask_again:;
	wchar_t *ws_linum = prompt_ask(L"goto line: ", NULL);
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

	frame_mv_csr(&editor_frames.data[editor_cur_frame], linum, 0);
}

void
editor_bind_del_fwd_ch(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	if (f->csr < f->buf->size)
		buf_erase(f->buf, f->csr, f->csr + 1);
}

void
editor_bind_del_back_ch(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];

	if (f->csr > 0 && f->buf->flags & BF_WRITABLE) {
		frame_mv_csr_rel(f, 0, -1, true);
		buf_erase(f->buf, f->csr, f->csr + 1);
		frame_comp_boundary(f);
	}
}

void
editor_bind_del_back_word(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	if (!(f->buf->flags & BF_WRITABLE))
		return;

	size_t ub = f->csr;

	while (f->csr > 0 && !iswalnum(buf_get_wch(f->buf, f->csr - 1)))
		--f->csr;
	while (f->csr > 0 && iswalnum(buf_get_wch(f->buf, f->csr - 1)))
		--f->csr;

	++f->csr;
	frame_mv_csr_rel(f, 0, -1, true);
	buf_erase(f->buf, f->csr, ub);
}

void
editor_bind_chg_mode_global(void)
{
	wchar_t *w_new_gm = prompt_ask(L"new globalmode: ", NULL);
	editor_redraw();
	if (!w_new_gm)
		return;

	size_t new_gm_size = sizeof(wchar_t) * (wcslen(w_new_gm) + 1);
	char *new_gm = malloc(new_gm_size);
	wcstombs(new_gm, w_new_gm, new_gm_size);
	free(w_new_gm);

	editor_reset_binds();
	mode_set(new_gm, &editor_frames.data[editor_cur_frame]);
	
	free(new_gm);
}

void
editor_bind_chg_mode_local(void)
{
	wchar_t *w_new_lm = prompt_ask(L"new frame localmode: ", NULL);
	editor_redraw();
	if (!w_new_lm)
		return;

	size_t new_lm_size = sizeof(wchar_t) * (wcslen(w_new_lm) + 1);
	char *new_lm = malloc(new_lm_size);
	wcstombs(new_lm, w_new_lm, new_lm_size);
	free(w_new_lm);

	free(editor_frames.data[editor_cur_frame].local_mode);
	editor_frames.data[editor_cur_frame].local_mode = strdup(new_lm);
	
	free(new_lm);
}

void
editor_bind_create_scrap(void)
{
	struct buf b = buf_create(true);
	struct frame f = frame_create(CONF_SCRAP_NAME, editor_add_buf(&b));
	editor_add_frame(&f);
	
	editor_cur_frame = editor_frames.size - 1;
	editor_reset_binds();
	editor_set_global_mode();
	editor_arrange_frames();
	editor_redraw();
}

void
editor_bind_new_line(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	buf_push_hist_brk(f->buf);
	buf_write_wch(f->buf, f->csr, L'\n');
	frame_mv_csr_rel(f, 0, !!(f->buf->flags & BF_WRITABLE), true);
}

void
editor_bind_focus(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	
	unsigned csrr, csrc;
	buf_pos(f->buf, f->csr, &csrr, &csrc);

	f->buf_start = 0;
	unsigned bsr = 0;
	long dst_bsr = (long)csrr - (f->sr - 1) / 2;
	dst_bsr = MAX(dst_bsr, 0);
	while (f->buf_start < f->buf->size && bsr < dst_bsr) {
		if (buf_get_wch(f->buf, f->buf_start++) == L'\n')
			++bsr;
	}

	frame_comp_boundary(f);
}

void
editor_bind_kill(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	if (!(f->buf->flags & BF_WRITABLE))
		return;
	
	if (editor_clipbuf)
		free(editor_clipbuf);
	
	size_t ln_end = f->csr;
	while (ln_end < f->buf->size && buf_get_wch(f->buf, ln_end) != L'\n')
		++ln_end;
	
	editor_clipbuf = malloc((ln_end - f->csr + 1) * sizeof(wchar_t));
	buf_get_wstr(f->buf, editor_clipbuf, f->csr, ln_end - f->csr + 1);
	
	buf_erase(f->buf, f->csr, ln_end);
}

void
editor_bind_paste(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	if (!(f->buf->flags & BF_WRITABLE))
		return;
	
	if (!editor_clipbuf) {
		prompt_show(L"clipbuffer is empty!");
		editor_redraw();
		return;
	}
	
	buf_write_wstr(f->buf, f->csr, editor_clipbuf);
	f->csr += wcslen(editor_clipbuf) + 1;
	frame_mv_csr_rel(f, 0, -1, true);
	frame_comp_boundary(f);
}

void
editor_bind_undo(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];

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

void
editor_bind_copy(void)
{
	if (editor_clipbuf)
		free(editor_clipbuf);
	
	struct frame *f = &editor_frames.data[editor_cur_frame];
	
	size_t ln = f->csr;
	while (ln > 0 && buf_get_wch(f->buf, ln - 1) != L'\n')
		--ln;
	
	size_t ln_end = f->csr;
	while (ln_end < f->buf->size && buf_get_wch(f->buf, ln_end) != L'\n')
		++ln_end;
	
	editor_clipbuf = malloc((ln_end - ln + 1) * sizeof(wchar_t));
	buf_get_wstr(f->buf, editor_clipbuf, ln, ln_end - ln + 1);
}

void
editor_bind_ncopy(void)
{
ask_again:;
	wchar_t *ws_ln_cnt = prompt_ask(L"copy lines: ", NULL);
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
	
	if (editor_clipbuf)
		free(editor_clipbuf);
	
	struct frame *f = &editor_frames.data[editor_cur_frame];
	
	size_t reg_lb = f->csr;
	while (reg_lb > 0 && buf_get_wch(f->buf, reg_lb - 1) != L'\n')
		--reg_lb;
	
	size_t reg_ub = f->csr;
	while (reg_ub < f->buf->size && ln_cnt > 0) {
		while (buf_get_wch(f->buf, reg_ub) != L'\n')
			++reg_ub;
		--ln_cnt;
		reg_ub += ln_cnt > 0;
	}
	
	editor_clipbuf = malloc((reg_ub - reg_lb + 1) * sizeof(wchar_t));
	buf_get_wstr(f->buf, editor_clipbuf, reg_lb, reg_ub - reg_lb + 1);
}

void
editor_bind_find_lit(void)
{
ask_again:;
	wchar_t *needle = prompt_ask(L"find literally: ", NULL);
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
	wchar_t *cmp_buf = malloc(sizeof(wchar_t) * (needle_len + 1));
	struct frame *f = &editor_frames.data[editor_cur_frame];
	for (search_pos = f->csr + 1; search_pos + needle_len <= f->buf->size; ++search_pos) {
		if (!wcscmp(buf_get_wstr(f->buf, cmp_buf, search_pos, needle_len + 1), needle))
			break;
	}
	
	free(cmp_buf);
	free(needle);
	
	if (search_pos + needle_len > f->buf->size) {
		prompt_show(L"did not find needle in haystack!");
		editor_redraw();
		return;
	}
	
	unsigned r, c;
	buf_pos(f->buf, search_pos, &r, &c);
	frame_mv_csr(f, r, c);
}

void
editor_bind_mac_begin(void)
{
	keybd_rec_mac_begin();
}

void
editor_bind_mac_end(void)
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

void
editor_bind_toggle_mono(void)
{
	editor_mono = !editor_mono;
	editor_arrange_frames();
	editor_redraw();
}

void
editor_bind_read_man_word(void)
{
	struct frame *f = &editor_frames.data[editor_cur_frame];
	
	// get label render parameters.
	unsigned csrr, csrc;
	frame_pos(f, f->csr, &csrr, &csrc);
	csrr += f->pr;
	csrc += f->pc;
	
	struct label_bounds bounds = {
		.pr = csrr,
		.pc = csrc + 1,
		.sr = CONF_READ_MAN_SR,
		.sc = CONF_READ_MAN_SC,
	};
	if (label_rebound(&bounds, csrc + 1, MAX(0, (long)csrc - 1), csrr)) {
		prompt_show(L"could not find valid anchoring for label!");
		editor_redraw();
		return;
	}
	
	// manpages generally have alphanumeric chars, underscores, hyphens, and
	// periods in their names; so only those are supported here.
	
	if (f->csr >= f->buf->size) {
		prompt_show(L"cannot get manpage for null name!");
		editor_redraw();
		return;
	}
	
	wchar_t wch = buf_get_wch(f->buf, f->csr);
	if (!iswalnum(wch) && !wcschr(L"_-.", wch)) {
		prompt_show(L"cannot get manpage for unsupported name!");
		editor_redraw();
		return;
	}
	
	// get currently hovered word.
	size_t word_start = f->csr;
	while (word_start > 0) {
		wchar_t wch = buf_get_wch(f->buf, word_start - 1);
		if (!iswalnum(wch) && !wcschr(L"_-.", wch))
			break;
		--word_start;
	}
	
	size_t word_len = 1;
	while (word_start + word_len < f->buf->size) {
		wchar_t wch = buf_get_wch(f->buf, word_start + word_len);
		if (!iswalnum(wch) && !wcschr(L"_-.", wch))
			break;
		++word_len;
	}
	
	size_t wword_size = sizeof(wchar_t) * (word_len + 1);
	wchar_t *wword = malloc(wword_size);
	buf_get_wstr(f->buf, wword, word_start, word_len + 1);
	
	char *word = malloc(wword_size);
	wcstombs(word, wword, wword_size);
	free(wword);
	
	// format word into command.
	char *cmd = malloc(2);
	size_t cmd_len = 0, cmd_cap = 1;
	size_t cmd_fmt_len = strlen(CONF_READ_MAN_CMD);
	
	for (size_t i = 0; i < cmd_fmt_len; ++i) {
		if (CONF_READ_MAN_CMD[i] != '%') {
			if (++cmd_len > cmd_cap) {
				cmd_cap *= 2;
				cmd = realloc(cmd, cmd_cap + 1);
			}
			cmd[cmd_len - 1] = CONF_READ_MAN_CMD[i];
			continue;
		}
		
		switch (CONF_READ_MAN_CMD[i + 1]) {
		case 0:
		case '%':
			if (++cmd_len > cmd_cap) {
				cmd_cap *= 2;
				cmd = realloc(cmd, cmd_cap + 1);
			}
			cmd[cmd_len - 1] = '%';
			break;
		case 'w':
			for (size_t j = 0; j < word_len; ++j) {
				if (++cmd_len > cmd_cap) {
					cmd_cap *= 2;
					cmd = realloc(cmd, cmd_cap + 1);
				}
				cmd[cmd_len - 1] = word[j];
			}
			break;
		default:
			break;
		}
		
		++i; // skip format code.
	}
	cmd[cmd_len] = 0;
	free(word);
	
	// read man pipe into message buffer.
	// terminal size is temporarily set to fit the label bounds in order to
	// trick man into generating output of a suitable size and spacing.
	// used temporary `winsize` has two extra columns to make man output
	// text without right-hand padding, which would be wasted space here.
	struct winsize sv_ws;
	ioctl(0, TIOCGWINSZ, &sv_ws);
	
	struct winsize tmp_ws = {
		.ws_row = bounds.sr,
		.ws_col = bounds.sc + 2,
	};
	editor_ignore_sigwinch = true;
	ioctl(0, TIOCSWINSZ, &tmp_ws);
	
	FILE *man_fp = popen(cmd, "r");
	free(cmd);
	
	if (!man_fp) {
		ioctl(0, TIOCSWINSZ, &sv_ws);
		editor_ignore_sigwinch = false;
		prompt_show(L"failed to open pipe to man!");
		editor_redraw();
		return;
	}
	
	// this should maybe eventually be changed.
	wchar_t *msg = malloc(sizeof(wchar_t));
	size_t msg_size = 0, msg_cap = 1;
	int ch;
	while ((ch = fgetc(man_fp)) != EOF) {
		if (++msg_size > msg_cap) {
			msg_cap *= 2;
			msg = realloc(msg, sizeof(wchar_t) * (msg_cap + 1));
		}
		msg[msg_size - 1] = ch;
	}
	msg[msg_size] = 0;
	ioctl(0, TIOCSWINSZ, &sv_ws);
	editor_ignore_sigwinch = false;
	
	if (pclose(man_fp)) {
		prompt_show(L"man exited with error!");
		editor_redraw();
		free(msg);
		return;
	}
	
	// draw label with message.
	editor_redraw();
	if (label_show(CONF_READ_MAN_TITLE, msg, &bounds)) {
		prompt_show(L"failed to show label!");
		editor_redraw();
		free(msg);
		return;
	}
	
	free(msg);
	editor_redraw();
}

void
editor_bind_file_exp(void)
{
	char open_path[PATH_MAX] = {0};
	
	editor_redraw();
	switch (file_exp_open(open_path, PATH_MAX - 1, ".")) {
	case FER_SUCCESS:
		break;
	case FER_FAIL:
		prompt_show(L"failed to open file explorer!");
		editor_redraw();
		return;
	case FER_QUIT:
		editor_redraw();
		return;
	}
	
	// TODO: open file.
}
