#include "frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>

#include <sys/ioctl.h>

#include "conf.h"
#include "draw.h"
#include "util.h"

// padding size around line numbers.
#define GUTTER (CONF_GUTTER_LEFT + CONF_GUTTER_RIGHT)

static void draw_line(struct frame const *f, unsigned *line, size_t *draw_csr);
static void exec_highlight(struct frame const *f, struct highlight const *hl);

VEC_DEF_IMPL(frame)

struct frame
frame_create(wchar_t const *name, struct buf *buf)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	unsigned ber, bec;
	buf_pos(buf, buf->size, &ber, &bec);
	unsigned linum_width = 0;
	for (unsigned i = MIN(ber, ws.ws_row - 1) + 1; i > 0; i /= 10)
		++linum_width;

	char *local_mode;
	if (buf->src_type == BST_FILE) {
		char const *buf_ext = file_ext(buf->src);
		for (size_t i = 0; i < conf_metab_size; ++i) {
			for (char const **ext = conf_metab[i].exts; *ext; ++ext) {
				if (!strcmp(*ext, buf_ext)) {
					local_mode = strdup(conf_metab[i].mode);
					goto done_find_lm;
				}
			}
		}
		local_mode = strdup("\0");
	done_find_lm:;
	} else
		local_mode = strdup("\0");
	
	return (struct frame){
		.name = wcsdup(name),
		.pr = 0,
		.pc = 0,
		.sr = ws.ws_row,
		.sc = ws.ws_col,
		.buf = buf,
		.csr = 0,
		.buf_start = 0,
		.csr_want_col = 0,
		.linum_width = linum_width,
		.local_mode = local_mode,
	};
}

void
frame_destroy(struct frame *f)
{
	free(f->name);
	free(f->local_mode);
}

void
frame_draw(struct frame const *f, bool active)
{
	unsigned bsr, bsc, ber, bec;
	buf_pos(f->buf, f->buf_start, &bsr, &bsc);
	buf_pos(f->buf, f->buf->size, &ber, &bec);

	unsigned left_edge = GUTTER + f->linum_width;

	// write frame name and buffer modification marker.
	size_t name_len = wcslen(f->name);
	for (unsigned i = 0; i < f->sc; ++i)
		draw_put_wch(f->pr, f->pc + i, i < name_len ? f->name[i] : L' ');

	if (f->buf->flags & BF_MODIFIED) {
		wchar_t mod_mk[] = CONF_BUF_MOD_MARK L"\0";
		size_t mod_mk_len = wcslen(mod_mk);
		if (f->sc >= 0 && f->sc < mod_mk_len + 1)
			mod_mk[f->sc] = 0;

		draw_put_wstr(f->pr, f->pc + f->sc - mod_mk_len, mod_mk);
	}

	draw_put_attr(f->pr, f->pc, active ? CONF_A_GHIGH : CONF_A_GNORM, f->sc);

	// fill frame.
	draw_fill(f->pr + 1, f->pc, f->sr - 1, f->sc, L' ', CONF_A_NORM);

	// write margins.
	unsigned befr, befc;
	frame_pos(f, f->buf->size, &befr, &befc);
	for (size_t i = 0; i < conf_mtab_size; ++i) {
		unsigned draw_col = left_edge + conf_mtab[i].col;
		if (draw_col >= f->sc)
			continue;
		
		for (unsigned j = 1, end = MIN(befr + 1, f->sr); j < end; ++j) {
			draw_put_wch(f->pr + j, f->pc + draw_col, conf_mtab[i].wch);
			draw_put_attr(f->pr + j, f->pc + draw_col, conf_mtab[i].attr, 1);
		}
	}

	// write lines and linums.
	size_t draw_csr = f->buf_start;
	unsigned linum_ind = 0;
	for (unsigned i = 1; i < f->sr; ++i) {
		if (linum_ind++ <= ber - bsr) {
			wchar_t draw_text[16];
			swprintf(draw_text, 16, L"%u", bsr + linum_ind);
			draw_put_wstr(f->pr + i, f->pc + CONF_GUTTER_LEFT + f->linum_width - wcslen(draw_text), draw_text);
			draw_put_attr(f->pr + i, f->pc, CONF_A_LINUM, GUTTER + f->linum_width);
		}

		draw_line(f, &i, &draw_csr);
	}

	// execute highlight.
	for (size_t i = 0; i < conf_htab_size; ++i) {
		if (!strcmp(conf_htab[i].local_mode, f->local_mode)) {
			exec_highlight(f, &conf_htab[i]);
			goto done_highlight;
		}
	}
done_highlight:;

	// draw cursor.
	unsigned csrr, csrc;
	frame_pos(f, f->csr, &csrr, &csrc);
	draw_put_attr(f->pr + csrr, f->pc + csrc, CONF_A_CURSOR, 1);
}

void
frame_pos(struct frame const *f, size_t pos, unsigned *out_r, unsigned *out_c)
{
	unsigned right_edge = f->sc - GUTTER - f->linum_width;
	
	*out_r = 1;
	*out_c = 0;

	for (size_t i = f->buf_start; i < pos && i < f->buf->size; ++i) {
		if (f->buf->conts[i] == L'\t')
			*out_c += CONF_TAB_SIZE - *out_c % CONF_TAB_SIZE - 1;

		if (f->buf->conts[i] == L'\n' || *out_c >= right_edge - 1) {
			*out_c = 0;
			++*out_r;
			continue;
		}

		++*out_c;
	}

	*out_c += GUTTER + f->linum_width;
}

void
frame_mv_csr(struct frame *f, unsigned r, unsigned c)
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

	frame_comp_boundary(f);
}

void
frame_mv_csr_rel(struct frame *f, int dr, int dc, bool wrap)
{
	int dc_sv = dc;
	
	if (wrap && dc != 0) {
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

	if (dc_sv != 0) {
		csrc = (long)csrc + dc < 0 ? 0 : csrc + dc;
		f->csr_want_col = csrc;
	} else
		csrc = f->csr_want_col;

	frame_mv_csr(f, csrr, csrc);
}

void
frame_comp_boundary(struct frame *f)
{
	unsigned left_edge = GUTTER + f->linum_width;
	unsigned right_edge = f->sc - left_edge;
	
	// redetermine buffer boundaries for rendering.
	unsigned csrr, csrc;
	frame_pos(f, f->csr, &csrr, &csrc);
	
	while (csrr >= f->sr) {
		++f->buf_start;
		while (f->buf_start < f->buf->size
		       && f->buf->conts[f->buf_start - 1] != L'\n') {
			++f->buf_start;
		}
		--csrr;
	}

	unsigned bsr, bsc;
	buf_pos(f->buf, f->buf_start, &bsr, &bsc);
	buf_pos(f->buf, f->csr, &csrr, &csrc);
	
	while (csrr < bsr) {
		--f->buf_start;
		while (f->buf_start > 0
		       && f->buf->conts[f->buf_start - 1] != L'\n') {
			--f->buf_start;
		}
		++csrr;
	}

	// fix linum width.
	unsigned ber, bec;
	buf_pos(f->buf, f->buf->size, &ber, &bec);
	f->linum_width = 0;
	for (unsigned i = MIN(ber, bsr + f->sr - 1) + 1; i > 0; i /= 10)
		++f->linum_width;
}

static void
draw_line(struct frame const *f, unsigned *line, size_t *draw_csr)
{
	unsigned left_edge = GUTTER + f->linum_width;
	unsigned right_edge = f->sc - left_edge;
	unsigned c = 0;

	while (*draw_csr < f->buf->size && f->buf->conts[*draw_csr] != L'\n') {
		if (c >= right_edge) {
			c = 0;
			++*line;
		}

		if (*line >= f->sr)
			break;

		wchar_t wch = f->buf->conts[*draw_csr];
		wch = wch == L'\t' || iswprint(wch) ? wch : 0xfffd;
		switch (wch) {
		case L'\t': {
			unsigned nch = CONF_TAB_SIZE - c % CONF_TAB_SIZE;
			nch = MIN(nch, right_edge - c);
			
			for (unsigned i = 0; i < nch && c + i < right_edge; ++i)
				draw_put_wch(f->pr + *line, f->pc + left_edge + c + i, L' ');
			draw_put_attr(f->pr + *line, f->pc + left_edge + c, CONF_A_NORM, nch);
			c += CONF_TAB_SIZE - c % CONF_TAB_SIZE;
			
			break;
		}
		default:
			draw_put_wch(f->pr + *line, f->pc + left_edge + c, wch);
			draw_put_attr(f->pr + *line, f->pc + left_edge + c, CONF_A_NORM, 1);
			++c;
			break;
		}
		
		++*draw_csr;
	}

	if (c >= right_edge)
		++*line;

	++*draw_csr;
}

static void
exec_highlight(struct frame const *f, struct highlight const *hl)
{
	unsigned left_edge = GUTTER + f->linum_width;
	
	size_t off = f->buf_start;
	size_t lb, ub;
	uint16_t a;
	while (!hl->find(f->buf->conts, f->buf->size, off, &lb, &ub, &a)) {
		unsigned hlr, hlc;
		frame_pos(f, lb, &hlr, &hlc);
		if (hlr >= f->sr)
			break;

		unsigned c = hlc - left_edge, r = hlr;
		for (size_t i = lb; i < ub; ++i) {
			if (c >= f->sc - left_edge) {
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
				w = CONF_TAB_SIZE - c % CONF_TAB_SIZE;
				break;
			default:
				w = 1;
				break;
			}

			draw_put_attr(f->pr + r, f->pc + left_edge + c, a, w);
			
			c += w;
		}

		off = ub;
	}
}
