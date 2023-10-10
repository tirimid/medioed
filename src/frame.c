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
	for (unsigned i = MIN(ber, ws.ws_row) + 1; i > 0; i /= 10)
		++linumw;

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
		.localmode = strdup(buf->srctype == BST_FILE ? fileext(buf->src) : "\0"),
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

			size_t dtlen = wcslen(drawtext);
			draw_putwstr(f->pr + i, f->pc + CONF_GUTTER_LEFT + f->linumw - dtlen, drawtext);
			draw_putattr(f->pr + i, f->pc, CONF_A_LINUM, GUTTER + dtlen);
		}

		drawline(f, &i, &dcsr);
	}

	// execute highlights.
	struct hbndry const *hbndry = NULL;
	for (size_t i = 0; i < conf_hbtab_size; ++i) {
		if (!strcmp(conf_hbtab[i].localmode, f->localmode)) {
			hbndry = &conf_hbtab[i];
			break;
		}
	}

	if (hbndry) {
		for (size_t i = hbndry->start; i < hbndry->end; ++i)
			exechighlight(f, &conf_htab[i]);
	}

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
	// adjust the actual cursor position.
	f->csr = 0;
	
	while (f->csr < f->buf->size && r > 0) {
		if (f->buf->conts[f->csr++] == L'\n')
			--r;
	}
	
	while (f->csr < f->buf->size && f->buf->conts[f->csr] != L'\n'
	       && c > 0) {
		++f->csr;
		--c;
	}

	// redetermine buffer boundaries for rendering.
	unsigned bsr, bsc, csrr, csrc;

	frame_pos(f, f->bufstart, &bsr, &bsc);
	frame_pos(f, f->csr, &csrr, &csrc);
	while (csrr >= bsr + f->sr - 1) {
		++f->bufstart;
		while (f->bufstart < f->buf->size
		       && f->buf->conts[f->bufstart - 1] != L'\n') {
			++f->bufstart;
		}

		frame_pos(f, f->bufstart, &bsr, &bsc);
		frame_pos(f, f->csr, &csrr, &csrc);
	}

	buf_pos(f->buf, f->bufstart, &bsr, &bsc);
	buf_pos(f->buf, f->csr, &csrr, &csrc);
	while (csrr < bsr) {
		--f->bufstart;
		while (f->bufstart > 0
		       && f->buf->conts[f->bufstart - 1] != L'\n') {
			--f->bufstart;
		}

		buf_pos(f->buf, f->bufstart, &bsr, &bsc);
		buf_pos(f->buf, f->csr, &csrr, &csrc);
	}

	// fix linum width.
	unsigned ber, bec;
	buf_pos(f->buf, f->buf->size, &ber, &bec);
	f->linumw = 0;
	for (unsigned i = bsr + MIN(ber - bsr, f->sr) + 1; i > 0; i /= 10)
		++f->linumw;
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

static void
drawline(struct frame const *f, unsigned *line, size_t *dcsr)
{
	unsigned ledge = GUTTER + f->linumw, redge = f->sc - ledge;
	unsigned i;

	for (i = 0; *dcsr < f->buf->size && f->buf->conts[*dcsr] != L'\n'; ++i, ++*dcsr) {
		if (i >= redge) {
			i = 0;
			++*line;
		}

		if (*line >= f->sr)
			break;

		wchar_t wch = f->buf->conts[*dcsr];
		switch (wch) {
		case L'\t':
			i += CONF_TABSIZE - i % CONF_TABSIZE - 1;
			break;
		default:
			draw_putwch(f->pr + *line, f->pc + ledge + i, wch);
			break;
		}
	}

	if (i >= redge)
		++*line;

	++*dcsr;
}

static void
exechighlight(struct frame const *f, struct highlight const *hl)
{
	pcre2_match_data *md = pcre2_match_data_create_from_pattern(hl->re, NULL);
	PCRE2_SIZE off = f->bufstart;
	unsigned ledge = GUTTER + f->linumw;
	while (pcre2_match(hl->re, (PCRE2_SPTR)f->buf->conts, f->buf->size, off, 0, md, NULL) >= 0) {
		PCRE2_SIZE const *offv = pcre2_get_ovector_pointer(md);

		unsigned hlr, hlc;
		frame_pos(f, offv[0], &hlr, &hlc);
		if (hlr >= f->sr)
			break;

		unsigned c = hlc - ledge, r = hlr;
		for (size_t i = offv[0]; i < offv[1]; ++i) {
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

			draw_putattr(f->pr + r, f->pc + ledge + c, hl->attr, w);
			
			c += w;
		}

		off = offv[1];
	}

	pcre2_match_data_free(md);
}
