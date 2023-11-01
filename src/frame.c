#include "frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>

#include <sys/ioctl.h>

#include "conf.h"
#include "draw.h"

// padding size around line numbers.
#define GUTTER (CONF_GUTTER_LEFT + CONF_GUTTER_RIGHT)

static void drawline(struct frame const *f, unsigned *line, size_t *dcsr);
static void exechighlight(struct frame const *f, struct highlight const *hl);

VEC_DEFIMPL(frame)

struct frame
frame_create(wchar_t const *name, struct buf *buf)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	unsigned ber, bec;
	buf_pos(buf, buf->size, &ber, &bec);
	unsigned linumw = 0;
	for (unsigned i = MIN(ber, ws.ws_row - 1) + 1; i > 0; i /= 10)
		++linumw;

	char *localmode;
	if (buf->srctype == BST_FILE) {
		char const *bufext = fileext(buf->src);
		for (size_t i = 0; i < conf_metab_size; ++i) {
			for (char const **ext = conf_metab[i].exts; *ext; ++ext) {
				if (!strcmp(*ext, bufext)) {
					localmode = strdup(conf_metab[i].mode);
					goto done_findlm;
				}
			}
		}
		localmode = strdup("\0");
	done_findlm:;
	} else
		localmode = strdup("\0");
	
	return (struct frame){
		.name = wcsdup(name),
		.pr = 0,
		.pc = 0,
		.sr = ws.ws_row,
		.sc = ws.ws_col,
		.buf = buf,
		.csr = 0,
		.bufstart = 0,
		.csr_wantcol = 0,
		.linumw = linumw,
		.localmode = localmode,
	};
}

void
frame_destroy(struct frame *f)
{
	free(f->name);
	free(f->localmode);
}

void
frame_draw(struct frame const *f, bool active)
{
	unsigned bsr, bsc, ber, bec;
	buf_pos(f->buf, f->bufstart, &bsr, &bsc);
	buf_pos(f->buf, f->buf->size, &ber, &bec);

	unsigned ledge = GUTTER + f->linumw;

	// write frame name and buffer modification marker.
	size_t namelen = wcslen(f->name);
	for (unsigned i = 0; i < f->sc; ++i)
		draw_putwch(f->pr, f->pc + i, i < namelen ? f->name[i] : L' ');

	if (f->buf->flags & BF_MODIFIED) {
		wchar_t modmk[] = CONF_BUFMODMARK L"\0";
		size_t modmklen = wcslen(modmk);
		if (f->sc >= 0 && f->sc < modmklen + 1)
			modmk[f->sc] = 0;

		draw_putwstr(f->pr, f->pc + f->sc - modmklen, modmk);
	}

	draw_putattr(f->pr, f->pc, active ? CONF_A_GHIGH : CONF_A_GNORM, f->sc);

	// fill frame.
	draw_fill(f->pr + 1, f->pc, f->sr - 1, f->sc, L' ', CONF_A_NORM);

	// write margins.
	unsigned befr, befc;
	frame_pos(f, f->buf->size, &befr, &befc);
	for (size_t i = 0; i < conf_mtab_size; ++i) {
		unsigned dcol = ledge + conf_mtab[i].col;
		if (dcol >= f->sc)
			continue;
		
		for (unsigned j = 1, end = MIN(befr + 1, f->sr); j < end; ++j) {
			draw_putwch(f->pr + j, f->pc + dcol, conf_mtab[i].wch);
			draw_putattr(f->pr + j, f->pc + dcol, conf_mtab[i].attr, 1);
		}
	}

	// write lines and linums.
	size_t dcsr = f->bufstart;
	unsigned linumind = 0;
	for (unsigned i = 1; i < f->sr; ++i) {
		if (linumind++ <= ber - bsr) {
			wchar_t drawtext[16];
			swprintf(drawtext, 16, L"%u", bsr + linumind);
			draw_putwstr(f->pr + i, f->pc + CONF_GUTTER_LEFT + f->linumw - wcslen(drawtext), drawtext);
			draw_putattr(f->pr + i, f->pc, CONF_A_LINUM, GUTTER + f->linumw);
		}

		drawline(f, &i, &dcsr);
	}

	// execute highlight.
	for (size_t i = 0; i < conf_htab_size; ++i) {
		if (!strcmp(conf_htab[i].localmode, f->localmode)) {
			exechighlight(f, &conf_htab[i]);
			goto donehl;
		}
	}
donehl:;

	// draw cursor.
	unsigned csrr, csrc;
	frame_pos(f, f->csr, &csrr, &csrc);
	draw_putattr(f->pr + csrr, f->pc + csrc, CONF_A_CURSOR, 1);
}

void
frame_pos(struct frame const *f, size_t pos, unsigned *out_r, unsigned *out_c)
{
	unsigned redge = f->sc - GUTTER - f->linumw;
	
	*out_r = 1;
	*out_c = 0;

	for (size_t i = f->bufstart; i < pos && i < f->buf->size; ++i) {
		if (f->buf->conts[i] == L'\t')
			*out_c += CONF_TABSIZE - *out_c % CONF_TABSIZE - 1;

		if (f->buf->conts[i] == L'\n' || *out_c >= redge - 1) {
			*out_c = 0;
			++*out_r;
			continue;
		}

		++*out_c;
	}

	*out_c += GUTTER + f->linumw;
}

void
frame_mvcsr(struct frame *f, unsigned r, unsigned c)
{
	f->csr = 0;
	
	while (f->csr < f->buf->size && r > 0) {
		if (f->buf->conts[f->csr++] == L'\n')
			--r;
	}
	
	while (f->csr < f->buf->size
	       && f->buf->conts[f->csr] != L'\n'
	       && c > 0) {
		++f->csr;
		--c;
	}

	frame_compbndry(f);
}

void
frame_relmvcsr(struct frame *f, int dr, int dc, bool lwrap)
{
	int dcsv = dc;
	
	if (lwrap && dc != 0) {
		int dir = SIGN(dc);
		long bs_dst = -(long)f->csr;
		long be_dst = f->buf->size - f->csr;

		dc = dir == -1 ? MAX(dc, bs_dst) : MIN(dc, be_dst);

		while (f->csr >= 0 && f->csr <= f->buf->size && dc != 0) {
			f->csr += dir;
			dc -= dir;
		}
	}

	unsigned csrr, csrc;

	buf_pos(f->buf, f->csr, &csrr, &csrc);
	csrr = (long)csrr + dr < 0 ? 0 : csrr + dr;

	if (dcsv != 0) {
		csrc = (long)csrc + dc < 0 ? 0 : csrc + dc;
		f->csr_wantcol = csrc;
	} else
		csrc = f->csr_wantcol;

	frame_mvcsr(f, csrr, csrc);
}

void
frame_compbndry(struct frame *f)
{
	unsigned ledge = GUTTER + f->linumw, redge = f->sc - ledge;
	
	// redetermine buffer boundaries for rendering.
	unsigned csrr, csrc;
	frame_pos(f, f->csr, &csrr, &csrc);
	
	while (csrr >= f->sr) {
		++f->bufstart;
		while (f->bufstart < f->buf->size
		       && f->buf->conts[f->bufstart - 1] != L'\n') {
			++f->bufstart;
		}
		--csrr;
	}

	unsigned bsr, bsc;
	buf_pos(f->buf, f->bufstart, &bsr, &bsc);
	buf_pos(f->buf, f->csr, &csrr, &csrc);
	
	while (csrr < bsr) {
		--f->bufstart;
		while (f->bufstart > 0
		       && f->buf->conts[f->bufstart - 1] != L'\n') {
			--f->bufstart;
		}
		++csrr;
	}

	// fix linum width.
	unsigned ber, bec;
	buf_pos(f->buf, f->buf->size, &ber, &bec);
	f->linumw = 0;
	for (unsigned i = MIN(ber, bsr + f->sr - 1) + 1; i > 0; i /= 10)
		++f->linumw;
}

static void
drawline(struct frame const *f, unsigned *line, size_t *dcsr)
{
	unsigned ledge = GUTTER + f->linumw, redge = f->sc - ledge;
	unsigned c = 0;

	while (*dcsr < f->buf->size && f->buf->conts[*dcsr] != L'\n') {
		if (c >= redge) {
			c = 0;
			++*line;
		}

		if (*line >= f->sr)
			break;

		wchar_t wch = f->buf->conts[*dcsr];
		wch = wch == L'\t' || iswprint(wch) ? wch : 0xfffd;
		switch (wch) {
		case L'\t': {
			unsigned nch = CONF_TABSIZE - c % CONF_TABSIZE;
			for (unsigned i = 0; i < nch; ++i)
				draw_putwch(f->pr + *line, f->pc + ledge + c + i, L' ');
			draw_putattr(f->pr + *line, f->pc + ledge + c, CONF_A_NORM, nch);
			c += CONF_TABSIZE - c % CONF_TABSIZE;
			break;
		}
		default:
			draw_putwch(f->pr + *line, f->pc + ledge + c, wch);
			draw_putattr(f->pr + *line, f->pc + ledge + c, CONF_A_NORM, 1);
			++c;
			break;
		}
		
		++*dcsr;
	}

	if (c >= redge)
		++*line;

	++*dcsr;
}

static void
exechighlight(struct frame const *f, struct highlight const *hl)
{
	unsigned ledge = GUTTER + f->linumw;
	
	size_t off = f->bufstart;
	size_t lb, ub;
	uint16_t a;
	while (!hl->find(f->buf->conts, f->buf->size, off, &lb, &ub, &a)) {
		unsigned hlr, hlc;
		frame_pos(f, lb, &hlr, &hlc);
		if (hlr >= f->sr)
			break;

		unsigned c = hlc - ledge, r = hlr;
		for (size_t i = lb; i < ub; ++i) {
			if (c >= f->sc - ledge) {
				c = 0;
				++r;
			}

			if (r >= f->sr)
				break;

			unsigned w;
			switch (f->buf->conts[i]) {
			case L'\n':
				c = 0;
				++r;
				continue;
			case L'\t':
				w = CONF_TABSIZE - c % CONF_TABSIZE;
				break;
			default:
				w = 1;
				break;
			}

			draw_putattr(f->pr + r, f->pc + ledge + c, a, w);
			
			c += w;
		}

		off = ub;
	}
}
