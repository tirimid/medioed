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
#include "minicalc.h"
#include "prompt.h"

extern bool flag_c;

static struct buf *addbuf(struct buf *b);
static struct frame *addframe(struct frame *f);
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
static void bind_navfwd_page(void);
static void bind_navback_ch(void);
static void bind_navback_word(void);
static void bind_navback_page(void);
static void bind_navdown(void);
static void bind_navup(void);
static void bind_navln_start(void);
static void bind_navln_end(void);
static void bind_navgoto(void);
static void bind_delfwd_ch(void);
static void bind_delback_ch(void);
static void bind_delback_word(void);
static void bind_chgmode_global(void);
static void bind_chgmode_local(void);
static void bind_create_scrap(void);
static void bind_newline(void);
static void bind_focus(void);
static void bind_kill(void);
static void bind_paste(void);
static void bind_undo(void);
static void bind_copy(void);
static void bind_ncopy(void);
static void bind_findlit(void);
static void bind_macbegin(void);
static void bind_macend(void);
static void bind_minicalc(void);
static void resetbinds(void);
static void setglobalmode(void);
static void sigwinch_handler(int arg);

static void (*old_sigwinch_handler)(int);
static bool running;
static size_t curframe;
static struct vec_frame frames;
static struct vec_pbuf pbufs;
static wchar_t *clipbuf = NULL;

int
editor_init(int argc, char const *argv[])
{
	keybd_init();

	frames = vec_frame_create();
	pbufs = vec_pbuf_create();
	curframe = 0;
	
	int firstarg = 1;
	while (firstarg < argc && *argv[firstarg] == '-')
		++firstarg;

	if (argc <= firstarg) {
		struct buf gb = buf_fromwstr(CONF_GREETTEXT, false);
		struct frame gf = frame_create(CONF_GREETNAME, addbuf(&gb));
		addframe(&gf);
	} else {
		for (int i = firstarg; i < argc; ++i) {
			draw_clear(L' ', CONF_A_GNORM);
			
			struct stat s;
			if ((stat(argv[i], &s) || !S_ISREG(s.st_mode)) && !flag_c) {
				// TODO: show which file failed to open.
				prompt_show(L"failed to open file!");
				continue;
			} else if (stat(argv[i], &s) && flag_c) {
				if (mkfile(argv[i])) {
					// TODO: show which file failed to be
					// created.
					prompt_show(L"failed to create file!");
					continue;
				}
			}
			
			size_t wnamen = strlen(argv[i]) + 1;
			wchar_t *wname = malloc(sizeof(wchar_t) * wnamen);
			mbstowcs(wname, argv[i], wnamen);

			struct buf b = buf_fromfile(argv[i]);
			struct frame f = frame_create(wname, addbuf(&b));
			addframe(&f);

			free(wname);
		}
	}

	if (frames.size == 0) {
		struct buf b = buf_create(true);
		struct frame f = frame_create(CONF_SCRAPNAME, addbuf(&b));
		addframe(&f);
	}

	struct sigaction sa;
	sigaction(SIGWINCH, NULL, &sa);
	old_sigwinch_handler = sa.sa_handler;
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);

	resetbinds();
	setglobalmode();
	arrangeframes();
	editor_redraw();

	return 0;
}

void
editor_mainloop(void)
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
		
		wint_t k = keybd_awaitkey();
		if (k != KEYBD_IGNORE && (k == L'\n' || k == L'\t' || iswprint(k))) {
			struct frame *f = &frames.data[curframe];
			
			buf_writewch(f->buf, f->csr, k);
			frame_relmvcsr(f, 0, !!(f->buf->flags & BF_WRITABLE), true);
			mode_keyupdate(k);
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

	for (size_t i = 0; i < pbufs.size; ++i) {
		buf_destroy(pbufs.data[i]);
		free(pbufs.data[i]);
	}

	vec_frame_destroy(&frames);
	vec_pbuf_destroy(&pbufs);

	keybd_quit();
}

void
editor_redraw(void)
{
	for (size_t i = 0; i < frames.size; ++i) {
		frame_compbndry(&frames.data[i]);
		frame_draw(&frames.data[i], i == curframe);
	}
	
	// draw current bind status if necessary.
	size_t len;
	if (!keybd_isexecmac() && keybd_curbind(NULL)
	    || keybd_isrecmac() && keybd_curmac(NULL)) {
		int const *src = keybd_isrecmac() ? keybd_curmac(&len) : keybd_curbind(&len);
		
		wchar_t dpy[(KEYBD_MAXDPYLEN + 1) * KEYBD_MAXMACLEN];
		keybd_keydpy(dpy, src, len);
		
		struct winsize ws;
		ioctl(0, TIOCGWINSZ, &ws);
		
		size_t dstart = 0, dlen = wcslen(dpy);
		while (dlen > ws.ws_col) {
			wchar_t const *next = wcschr(dpy + dstart, L' ') + 1;
			if (!next)
				break;
			
			size_t diff = (uintptr_t)next - (uintptr_t)(dpy + dstart);
			size_t ndiff = diff / sizeof(wchar_t);
			
			dstart += ndiff;
			dlen -= ndiff;
		}
		
		draw_putwstr(ws.ws_row - 1, 0, dpy + dstart);
		draw_putattr(ws.ws_row - 1, 0, CONF_A_GHIGH, dlen);
	}
	
	draw_refresh();
}

static struct buf *
addbuf(struct buf *b)
{
	if (b->srctype == BST_FILE) {
		for (size_t i = 0; i < pbufs.size; ++i) {
			if (pbufs.data[i]->srctype == BST_FILE
			    && !strcmp(b->src, pbufs.data[i]->src)) {
				buf_destroy(b);
				return pbufs.data[i];
			}
		}
	}

	struct buf *pb = malloc(sizeof(struct buf));
	*pb = *b;
	vec_pbuf_add(&pbufs, &pb);
	
	return pbufs.data[pbufs.size - 1];
}

static struct frame *
addframe(struct frame *f)
{
	vec_frame_add(&frames, f);
	return &frames.data[frames.size - 1];
}

static void
arrangeframes(void)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	struct frame *f = &frames.data[0];

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
	bool modexists = false;
	for (size_t i = 0; i < pbufs.size; ++i) {
		if ((*pbufs.data[i]).flags & BF_MODIFIED) {
			modexists = true;
			break;
		}
	}

	if (modexists) {
		int confirm = prompt_yesno(L"there are unsaved modified buffers! quit anyway?", false);
		editor_redraw();
		if (confirm != 1)
			return;
	}
	
	running = false;
}

static void
bind_chgfwd_frame(void)
{
	curframe = (curframe + 1) % frames.size;
	resetbinds();
	setglobalmode();
	editor_redraw();
}

static void
bind_chgback_frame(void)
{
	curframe = (curframe == 0 ? frames.size : curframe) - 1;
	resetbinds();
	setglobalmode();
	editor_redraw();
}

static void
bind_focus_frame(void)
{
	if (curframe != 0) {
		vec_frame_swap(&frames, 0, curframe);
		curframe = 0;
		resetbinds();
		setglobalmode();
		arrangeframes();
		editor_redraw();
	}
}

static void
bind_kill_frame(void)
{
	int confirm = prompt_yesno(L"kill active frame?", false);
	editor_redraw();
	if (confirm != 1)
		return;
	
	frame_destroy(&frames.data[curframe]);
	vec_frame_rm(&frames, curframe);
	curframe = curframe > 0 ? curframe - 1 : 0;

	// destroy orphaned buffers.
	for (size_t i = 0; i < pbufs.size; ++i) {
		bool orphan = true;
		for (size_t j = 0; j < frames.size; ++j) {
			if (pbufs.data[i] == frames.data[j].buf) {
				orphan = false;
				break;
			}
		}

		if (orphan) {
			buf_destroy(pbufs.data[i]);
			free(pbufs.data[i]);
			vec_pbuf_rm(&pbufs, i--);
		}
	}

	if (frames.size == 0) {
		struct buf b = buf_create(true);
		struct frame f = frame_create(CONF_SCRAPNAME, addbuf(&b));
		addframe(&f);
	}

	resetbinds();
	setglobalmode();
	arrangeframes();
	editor_redraw();
}

static void
bind_open_file(void)
{
	wchar_t *wpath = prompt_ask(L"open file: ", prompt_comp_path, NULL);
	editor_redraw();
	if (!wpath)
		return;

	size_t pathsize = sizeof(wchar_t) * (wcslen(wpath) + 1);
	char *path = malloc(pathsize);
	wcstombs(path, wpath, pathsize);

	struct stat s;
	if (stat(path, &s) || !S_ISREG(s.st_mode)) {
		prompt_show(L"could not open file!");
		editor_redraw();
		free(path);
		return;
	}

	struct buf buf = buf_fromfile(path);
	struct frame frame = frame_create(wpath, addbuf(&buf));
	addframe(&frame);

	curframe = frames.size - 1;
	resetbinds();
	setglobalmode();
	arrangeframes();
	editor_redraw();

	free(path);
	free(wpath);
}

static void
bind_save_file(void)
{
	struct frame *f = &frames.data[curframe];
	enum bufsrctype prevtype = f->buf->srctype;
	uint8_t prevflags = f->buf->flags;

	wchar_t *wpath;
	if (f->buf->srctype == BST_FRESH) {
		f->buf->srctype = BST_FILE;
		
		wpath = prompt_ask(L"save to file: ", prompt_comp_path, NULL);
		editor_redraw();

		if (wpath) {
			size_t pathsize = sizeof(wchar_t) * (wcslen(wpath) + 1);
			char *path = malloc(pathsize);
			wcstombs(path, wpath, pathsize);

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
		f->buf->srctype = prevtype;
		f->buf->flags = prevflags;
		
		if (wpath)
			free(wpath);
		
		return;
	}
	
	if (prevtype == BST_FRESH)
		mkfile(f->buf->src);

	if (buf_save(f->buf)) {
		prompt_show(L"failed to write file!");
		editor_redraw();
		free(wpath);
		return;
	}

	if (prevtype == BST_FRESH) {
		free(f->name);
		f->name = wpath;
	}

	if (!f->localmode[0]) {
		free(f->localmode);
		f->localmode = strdup(fileext(f->buf->src));
	}

	if (!mode_get())
		setglobalmode();
}

static void
bind_navfwd_ch(void)
{
	frame_relmvcsr(&frames.data[curframe], 0, 1, true);
}

static void
bind_navfwd_word(void)
{
	struct frame *f = &frames.data[curframe];

	while (f->csr < f->buf->size && !iswalnum(f->buf->conts[f->csr]))
		++f->csr;
	while (f->csr < f->buf->size && iswalnum(f->buf->conts[f->csr]))
		++f->csr;
	
	++f->csr;
	frame_relmvcsr(f, 0, -1, true);
}

static void
bind_navfwd_page(void)
{
	struct frame *f = &frames.data[curframe];

	f->csr = f->bufstart;
	frame_relmvcsr(f, f->sr - 1, 0, false);
	
	f->bufstart = f->csr;
	while (f->bufstart > 0 && f->buf->conts[f->bufstart - 1] != L'\n')
		--f->bufstart;

	frame_compbndry(f);
}

static void
bind_navback_ch(void)
{
	frame_relmvcsr(&frames.data[curframe], 0, -1, true);
}

static void
bind_navback_word(void)
{
	struct frame *f = &frames.data[curframe];

	while (f->csr > 0 && !iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;
	while (f->csr > 0 && iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;

	++f->csr;
	frame_relmvcsr(f, 0, -1, true);
}

static void
bind_navback_page(void)
{
	struct frame *f = &frames.data[curframe];
	f->csr = f->bufstart;
	frame_relmvcsr(f, 1 - f->sr, 0, false);
}

static void
bind_navdown(void)
{
	frame_relmvcsr(&frames.data[curframe], 1, 0, false);
}

static void
bind_navup(void)
{
	frame_relmvcsr(&frames.data[curframe], -1, 0, false);
}

static void
bind_navln_start(void)
{
	frame_relmvcsr(&frames.data[curframe], 0, -INT_MAX, false);
}

static void
bind_navln_end(void)
{
	frame_relmvcsr(&frames.data[curframe], 0, INT_MAX, false);
}

static void
bind_navgoto(void)
{
askagain:;
	wchar_t *wslinum = prompt_ask(L"goto line: ", NULL, NULL);
	editor_redraw();
	if (!wslinum)
		return;

	if (!*wslinum) {
		free(wslinum);
		prompt_show(L"expected a line number!");
		editor_redraw();
		goto askagain;
	}

	size_t slinumsize = sizeof(wchar_t) * (wcslen(wslinum) + 1);
	char *slinum = malloc(slinumsize);
	wcstombs(slinum, wslinum, slinumsize);
	free(wslinum);

	for (char const *c = slinum; *c; ++c) {
		if (!isdigit(*c)) {
			free(slinum);
			prompt_show(L"invalid line number!");
			editor_redraw();
			goto askagain;
		}
	}

	unsigned linum = atoi(slinum);
	free(slinum);
	linum -= linum != 0;

	frame_mvcsr(&frames.data[curframe], linum, 0);
}

static void
bind_delfwd_ch(void)
{
	struct frame *f = &frames.data[curframe];
	if (f->csr < f->buf->size)
		buf_erase(f->buf, f->csr, f->csr + 1);
}

static void
bind_delback_ch(void)
{
	struct frame *f = &frames.data[curframe];

	if (f->csr > 0 && f->buf->flags & BF_WRITABLE) {
		frame_relmvcsr(f, 0, -1, true);
		buf_erase(f->buf, f->csr, f->csr + 1);
		frame_compbndry(f);
	}
}

static void
bind_delback_word(void)
{
	struct frame *f = &frames.data[curframe];
	if (!(f->buf->flags & BF_WRITABLE))
		return;

	size_t ub = f->csr;

	while (f->csr > 0 && !iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;
	while (f->csr > 0 && iswalnum(f->buf->conts[f->csr - 1]))
		--f->csr;

	++f->csr;
	frame_relmvcsr(f, 0, -1, true);
	buf_erase(f->buf, f->csr, ub);
}

static void
bind_chgmode_global(void)
{
	wchar_t *wnewgm = prompt_ask(L"new globalmode: ", NULL, NULL);
	editor_redraw();
	if (!wnewgm)
		return;

	size_t newgmsize = sizeof(wchar_t) * (wcslen(wnewgm) + 1);
	char *newgm = malloc(newgmsize);
	wcstombs(newgm, wnewgm, newgmsize);
	free(wnewgm);

	resetbinds();
	mode_set(newgm, &frames.data[curframe]);
	
	free(newgm);
}

static void
bind_chgmode_local(void)
{
	wchar_t *wnewlm = prompt_ask(L"new frame localmode: ", NULL, NULL);
	editor_redraw();
	if (!wnewlm)
		return;

	size_t newlmsize = sizeof(wchar_t) * (wcslen(wnewlm) + 1);
	char *newlm = malloc(newlmsize);
	wcstombs(newlm, wnewlm, newlmsize);
	free(wnewlm);

	free(frames.data[curframe].localmode);
	frames.data[curframe].localmode = strdup(newlm);
	
	free(newlm);
}

static void
bind_create_scrap(void)
{
	struct buf b = buf_create(true);
	struct frame f = frame_create(CONF_SCRAPNAME, addbuf(&b));
	addframe(&f);
	
	curframe = frames.size - 1;
	resetbinds();
	setglobalmode();
	arrangeframes();
	editor_redraw();
}

static void
bind_newline(void)
{
	struct frame *f = &frames.data[curframe];
	buf_pushhistbrk(f->buf);
	buf_writewch(f->buf, f->csr, L'\n');
	frame_relmvcsr(f, 0, !!(f->buf->flags & BF_WRITABLE), true);
}

static void
bind_focus(void)
{
	struct frame *f = &frames.data[curframe];
	
	unsigned csrr, csrc;
	buf_pos(f->buf, f->csr, &csrr, &csrc);

	f->bufstart = 0;
	unsigned bsr = 0;
	long dstbsr = (long)csrr - (f->sr - 1) / 2;
	dstbsr = MAX(dstbsr, 0);
	while (f->bufstart < f->buf->size && bsr < dstbsr) {
		if (f->buf->conts[f->bufstart++] == L'\n')
			++bsr;
	}

	frame_compbndry(f);
}

static void
bind_kill(void)
{
	struct frame *f = &frames.data[curframe];
	if (!(f->buf->flags & BF_WRITABLE))
		return;
	
	if (clipbuf)
		free(clipbuf);
	
	size_t lnend = f->csr;
	while (lnend < f->buf->size && f->buf->conts[lnend] != L'\n')
		++lnend;
	
	size_t cpsize = (lnend - f->csr) * sizeof(wchar_t);
	clipbuf = malloc(cpsize + sizeof(wchar_t));
	clipbuf[lnend - f->csr] = 0;
	memcpy(clipbuf, f->buf->conts + f->csr, cpsize);
	
	buf_erase(f->buf, f->csr, lnend);
}

static void
bind_paste(void)
{
	struct frame *f = &frames.data[curframe];
	if (!(f->buf->flags & BF_WRITABLE))
		return;
	
	if (!clipbuf) {
		prompt_show(L"clipbuffer is empty!");
		editor_redraw();
		return;
	}
	
	buf_writewstr(f->buf, f->csr, clipbuf);
	f->csr += wcslen(clipbuf);
	frame_compbndry(f);
}

static void
bind_undo(void)
{
	struct frame *f = &frames.data[curframe];

	if (f->buf->hist.size == 0) {
		prompt_show(L"no further undo information!");
		editor_redraw();
		return;
	}

	struct bufop const *bo = &f->buf->hist.data[f->buf->hist.size - 1];
	while (bo > f->buf->hist.data && bo->type == BOT_BRK)
		--bo;
	
	size_t csrdst;
	switch (bo->type) {
	case BOT_WRITE:
		csrdst = bo->lb;
		break;
	case BOT_ERASE:
		csrdst = bo->ub;
		break;
	}

	if (buf_undo(f->buf)) {
		prompt_show(L"failed to undo last operation!");
		editor_redraw();
		return;
	}

	unsigned csrr, csrc;
	buf_pos(f->buf, csrdst, &csrr, &csrc);
	frame_mvcsr(f, csrr, csrc);
	f->csr_wantcol = csrc;
}

static void
bind_copy(void)
{
	if (clipbuf)
		free(clipbuf);
	
	struct frame *f = &frames.data[curframe];
	
	size_t ln = f->csr;
	while (ln > 0 && f->buf->conts[ln - 1] != L'\n')
		--ln;
	
	size_t lnend = f->csr;
	while (lnend < f->buf->size && f->buf->conts[lnend] != L'\n')
		++lnend;
	
	size_t cpsize = (lnend - ln) * sizeof(wchar_t);
	clipbuf = malloc(cpsize + sizeof(wchar_t));
	clipbuf[lnend - ln] = 0;
	memcpy(clipbuf, f->buf->conts + ln, cpsize);
}

static void
bind_ncopy(void)
{
askagain:;
	wchar_t *wslcnt = prompt_ask(L"copy lines: ", NULL, NULL);
	editor_redraw();
	if (!wslcnt)
		return;
	
	if (!*wslcnt) {
		free(wslcnt);
		prompt_show(L"expected a line count!");
		editor_redraw();
		goto askagain;
	}
	
	size_t slcntsize = sizeof(wchar_t) * (wcslen(wslcnt) + 1);
	char *slcnt = malloc(slcntsize);
	wcstombs(slcnt, wslcnt, slcntsize);
	free(wslcnt);
	
	for (char const *c = slcnt; *c; ++c) {
		if (!isdigit(*c)) {
			free(slcnt);
			prompt_show(L"invalid line count!");
			editor_redraw();
			goto askagain;
		}
	}
	
	unsigned lcnt = atoi(slcnt);
	free(slcnt);
	if (!lcnt) {
		prompt_show(L"cannot copy zero lines!");
		editor_redraw();
		goto askagain;
	}
	
	if (clipbuf)
		free(clipbuf);
	
	struct frame *f = &frames.data[curframe];
	
	size_t reglb = f->csr;
	while (reglb > 0 && f->buf->conts[reglb - 1] != L'\n')
		--reglb;
	
	size_t regub = f->csr;
	while (regub < f->buf->size && lcnt > 0) {
		while (f->buf->conts[regub] != L'\n')
			++regub;
		--lcnt;
		regub += lcnt > 0;
	}
	
	size_t cpsize = (regub - reglb) * sizeof(wchar_t);
	clipbuf = malloc(cpsize + sizeof(wchar_t));
	clipbuf[regub - reglb] = 0;
	memcpy(clipbuf, f->buf->conts + reglb, cpsize);
}

static void
bind_findlit(void)
{
askagain:;
	wchar_t *needle = prompt_ask(L"find literally: ", NULL, NULL);
	editor_redraw();
	if (!needle)
		return;
	
	if (!*needle) {
		free(needle);
		prompt_show(L"expected a needle!");
		editor_redraw();
		goto askagain;
	}
	
	size_t spos, needlen = wcslen(needle);
	struct frame *f = &frames.data[curframe];
	for (spos = f->csr + 1; spos < f->buf->size; ++spos) {
		if (!wcsncmp(&f->buf->conts[spos], needle, needlen))
			break;
	}
	
	free(needle);
	
	if (spos == f->buf->size) {
		prompt_show(L"did not find needle in haystack!");
		editor_redraw();
		return;
	}
	
	unsigned r, c;
	buf_pos(f->buf, spos, &r, &c);
	frame_mvcsr(f, r, c);
}

static void
bind_macbegin(void)
{
	keybd_recmac_begin();
}

static void
bind_macend(void)
{
	if (keybd_isrecmac())
		keybd_recmac_end();
	else {
		keybd_recmac_end();
		
		if (!keybd_curmac(NULL)) {
			prompt_show(L"no macro specified to execute!");
			editor_redraw();
			return;
		}
		
		keybd_execmac();
	}
}

static void
bind_minicalc(void)
{
	wchar_t *expr = prompt_ask(L"evaluate expression: ", NULL, NULL);
	editor_redraw();
	if (!expr)
		return;
	
	wchar_t out[512];
	int rc = minicalc_eval(out, 512, expr);
	free(expr);
	if (rc)
		return;
	
	prompt_show(out);
	editor_redraw();
}

static void
resetbinds(void)
{
	// quit and reinit to reset current keybind buffer and bind information.
	keybd_quit();
	keybd_init();

	keybd_bind(conf_bind_quit, bind_quit);
	keybd_bind(conf_bind_chgfwd_frame, bind_chgfwd_frame);
	keybd_bind(conf_bind_chgback_frame, bind_chgback_frame);
	keybd_bind(conf_bind_focus_frame, bind_focus_frame);
	keybd_bind(conf_bind_kill_frame, bind_kill_frame);
	keybd_bind(conf_bind_open_file, bind_open_file);
	keybd_bind(conf_bind_save_file, bind_save_file);
	keybd_bind(conf_bind_navfwd_ch, bind_navfwd_ch);
	keybd_bind(conf_bind_navfwd_word, bind_navfwd_word);
	keybd_bind(conf_bind_navfwd_page, bind_navfwd_page);
	keybd_bind(conf_bind_navback_ch, bind_navback_ch);
	keybd_bind(conf_bind_navback_word, bind_navback_word);
	keybd_bind(conf_bind_navback_page, bind_navback_page);
	keybd_bind(conf_bind_navdown, bind_navdown);
	keybd_bind(conf_bind_navup, bind_navup);
	keybd_bind(conf_bind_navln_start, bind_navln_start);
	keybd_bind(conf_bind_navln_end, bind_navln_end);
	keybd_bind(conf_bind_navgoto, bind_navgoto);
	keybd_bind(conf_bind_delfwd_ch, bind_delfwd_ch);
	keybd_bind(conf_bind_delback_ch, bind_delback_ch);
	keybd_bind(conf_bind_delback_word, bind_delback_word);
	keybd_bind(conf_bind_chgmode_global, bind_chgmode_global);
	keybd_bind(conf_bind_chgmode_local, bind_chgmode_local);
	keybd_bind(conf_bind_create_scrap, bind_create_scrap);
	keybd_bind(conf_bind_newline, bind_newline);
	keybd_bind(conf_bind_focus, bind_focus);
	keybd_bind(conf_bind_kill, bind_kill);
	keybd_bind(conf_bind_paste, bind_paste);
	keybd_bind(conf_bind_undo, bind_undo);
	keybd_bind(conf_bind_copy, bind_copy);
	keybd_bind(conf_bind_ncopy, bind_ncopy);
	keybd_bind(conf_bind_findlit, bind_findlit);
	keybd_bind(conf_bind_macbegin, bind_macbegin);
	keybd_bind(conf_bind_macend, bind_macend);
	keybd_bind(conf_bind_minicalc, bind_minicalc);
	
	keybd_organize();
}

static void
setglobalmode(void)
{
	struct frame *f = &frames.data[curframe];
	if (f->buf->srctype != BST_FILE)
		return;

	char const *bufext = fileext(f->buf->src);
	for (size_t i = 0; i < conf_metab_size; ++i) {
		for (char const **ext = conf_metab[i].exts; *ext; ++ext) {
			if (!strcmp(bufext, *ext)) {
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
	
	arrangeframes();
	
	for (size_t i = 0; i < frames.size; ++i)
		frame_compbndry(&frames.data[i]);
	
	editor_redraw();
}
