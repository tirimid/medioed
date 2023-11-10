#include "editor.h"

#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
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

static struct buf *addbuf(struct buf *b);
static struct frame *addframe(struct frame *f);
static void arrangeframes(void);
static void redrawall(void);
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
static void resetbinds(void);
static void setglobalmode(void);
static void sigwinch_handler(int arg);

static void (*old_sigwinch_handler)(int);
static bool running;
static size_t curframe;
static struct vec_frame frames;
static struct vec_pbuf pbufs;

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
			if (stat(argv[i], &s) || !S_ISREG(s.st_mode)) {
				// TODO: show which file failed to open.
				prompt_show(L"failed to open file!");
				continue;
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
	redrawall();

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

		for (size_t i = 0; i < frames.size; ++i) {
			if (frames.data[i].buf == frames.data[curframe].buf)
				frame_draw(&frames.data[i], i == curframe);
		}
		draw_refresh();
		
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
redrawall(void)
{
	for (size_t i = 0; i < frames.size; ++i) {
		frame_compbndry(&frames.data[i]);
		frame_draw(&frames.data[i], i == curframe);
	}
	
	draw_refresh();
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
		redrawall();
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
	redrawall();
}

static void
bind_chgback_frame(void)
{
	curframe = (curframe == 0 ? frames.size : curframe) - 1;
	resetbinds();
	setglobalmode();
	redrawall();
}

static void
bind_focus_frame(void)
{
	vec_frame_swap(&frames, 0, curframe);
	curframe = 0;
	arrangeframes();
	redrawall();
}

static void
bind_kill_frame(void)
{
	int confirm = prompt_yesno(L"kill active frame?", false);
	redrawall();
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
	redrawall();
}

static void
bind_open_file(void)
{
	wchar_t *wpath = prompt_ask(L"open file: ", prompt_comp_path, NULL);
	redrawall();
	if (!wpath)
		return;

	size_t pathsize = sizeof(wchar_t) * (wcslen(wpath) + 1);
	char *path = malloc(pathsize);
	wcstombs(path, wpath, pathsize);

	struct stat s;
	if (stat(path, &s) || !S_ISREG(s.st_mode)) {
		prompt_show(L"could not open file!");
		redrawall();
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
	redrawall();

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
		redrawall();

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
		redrawall();
	}
	
	if (!f->buf->src || !*(uint8_t *)f->buf->src) {
		f->buf->srctype = prevtype;
		f->buf->flags = prevflags;
		
		if (wpath)
			free(wpath);
		
		return;
	}

	if (buf_save(f->buf)) {
		prompt_show(L"failed to write file!");
		redrawall();
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
	redrawall();
	if (!wslinum)
		return;

	if (!*wslinum) {
		free(wslinum);
		prompt_show(L"expected a line number!");
		redrawall();
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
			redrawall();
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
	redrawall();
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
	redrawall();
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
	redrawall();
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
	prompt_show(L"C-k is not implemented yet!");
	redrawall();
}

static void
bind_paste(void)
{
	prompt_show(L"C-y is not implemented yet!");
	redrawall();
}

static void
bind_undo(void)
{
	struct frame *f = &frames.data[curframe];

	if (f->buf->hist.size == 0) {
		prompt_show(L"no further undo information!");
		redrawall();
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
		redrawall();
		return;
	}

	unsigned csrr, csrc;
	buf_pos(f->buf, csrdst, &csrr, &csrc);
	frame_mvcsr(f, csrr, csrc);
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
	
	redrawall();
}
