#include "mode/mode_c.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"
#include "mode/mutil.h"
#include "util.h"

#define CNS_CONTINUE 1

struct rd_state {
	bool in_blk_cmt, in_ln_cmt;
	bool in_ch, in_str;
};

static void bind_indent(void);
static void bind_new_line(void);
static unsigned comp_tabs(size_t first_ch, size_t last_sig_ch);
static unsigned comp_smart_spaces(size_t first_spc, size_t off, size_t ln, size_t first_ch);
static long nopen_at(size_t pos, wchar_t open, wchar_t close);
static int comp_next_state(size_t pos, size_t *off, struct rd_state *rds);
static bool not_iswspace(wchar_t wch);
static bool is_sig_last(wchar_t wch);
static struct vec_mu_region get_skip_regs(void);

static struct frame *mf;

static int c_bind_indent[] = {K_TAB, -1};

static wchar_t const *indent_kw[] = {
	L"do",
	L"else",
};

void
mode_c_init(struct frame *f)
{
	mf = f;
	mu_init(f);

	mu_set_base();
	mu_set_pairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_SQUOTE | PF_DQUOTE);

	keybd_bind(c_bind_indent, bind_indent);
	keybd_bind(conf_bind_new_line, bind_new_line);
	
	keybd_organize();
}

void
mode_c_quit(void)
{
}

void
mode_c_update(void)
{
}

void
mode_c_keypress(wint_t k)
{
}

static void
bind_indent(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
	wchar_t const *src = mf->buf->conts;
	struct vec_mu_region skip = get_skip_regs();
	
	size_t ln = mu_get_ln(mf->csr, NULL);
	size_t ln_end = mu_get_ln_end(mf->csr, NULL);
	size_t prev_ln = mu_get_prev_ln(ln, NULL);
	size_t first_ch = mu_get_first(ln, NULL, not_iswspace);
	size_t last_sig_ch = mu_get_last(ln_end, &skip, is_sig_last);
	
	unsigned ntab = comp_tabs(first_ch, last_sig_ch), nspace = 0;
	
	if (prev_ln != ln) {
		size_t prev_ln_end = mu_get_ln_end(prev_ln, NULL);
		size_t prev_first_ch = mu_get_first(prev_ln, NULL, not_iswspace);
		size_t prev_last_ch = mu_get_last(prev_ln_end, &skip, not_iswspace);
		
		if (src[prev_last_ch] == L')' && src[prev_first_ch] != L'#') {
			if (ntab > 0
			    && nopen_at(prev_last_ch + 1, L'(', L')') == 0
			    && !wcschr(L"{}", src[first_ch])) {
				++ntab;
			}
		} else if (src[prev_last_ch] == L'\\')
			++ntab;

		for (size_t i = 0; i < ARRAY_SIZE(indent_kw); ++i) {
			size_t len = wcslen(indent_kw[i]);
			
			if (len <= prev_last_ch
			    && !iswalnum(src[prev_last_ch - len])
			    && !wcsncmp(indent_kw[i], &src[prev_last_ch - len + 1], len)) {
				++ntab;
				break;
			}
		}
		
		unsigned prev_ntab = 0, off = 0;
		size_t first_spc = prev_ln;
		
		while (src[first_spc] == L'\t') {
			++first_spc;
			++prev_ntab;
		}
		while (src[first_spc + off] == L' ')
			++off;
		
		if (first_ch < mf->buf->size
		    && !wcschr(L"{}", src[first_ch])
		    && !wcschr(L";/", src[prev_last_ch])
		    && off != 0
		    && !iswspace(src[first_spc + off])
		    && ntab == prev_ntab) {
			nspace = off;
		} else if (off == 0 && ntab == prev_ntab)
			nspace = comp_smart_spaces(first_spc, off, ln, first_ch);
	}
	
	mu_finish_indent(ln, first_ch, ntab, nspace);
	vec_mu_region_destroy(&skip);
}

static void
bind_new_line(void)
{
	bind_indent();

	buf_push_hist_brk(mf->buf);
	buf_write_wch(mf->buf, mf->csr, L'\n');
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
	
	bind_indent();
}

static unsigned
comp_tabs(size_t first_ch, size_t last_sig_ch)
{
	wchar_t const *src = mf->buf->conts;
	
	unsigned ntab = 0;
	
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	
	struct stk_unsigned pause_stk_open = stk_unsigned_create();
	struct stk_unsigned pause_stk_close = stk_unsigned_create();
	size_t prev_npause = 0;
	
	for (size_t i = 0; i < first_ch; ++i) {
		wchar_t wch = src[i];
		
		if (comp_next_state(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		if (rds.in_str || rds.in_ch || rds.in_blk_cmt || rds.in_ln_cmt)
			continue;
		else if (wch == L'\n') {
			if (pause_stk_open.size > prev_npause) {
				free(stk_unsigned_pop(&pause_stk_open));
				free(stk_unsigned_pop(&pause_stk_close));
			}
		} else if (wch == L'{') {
			if (pause_stk_open.size > 0
			    && *stk_unsigned_peek(&pause_stk_open) == ntab) {
				free(stk_unsigned_pop(&pause_stk_open));
			} else
				++ntab;
		} else if (wch == L'}' && ntab > 0) {
			if (pause_stk_close.size > 0
			    && *stk_unsigned_peek(&pause_stk_close) == ntab) {
				free(stk_unsigned_pop(&pause_stk_close));
			} else
				ntab -= ntab > 0;
		} else if (wch == L':') {
			stk_unsigned_push(&pause_stk_open, &ntab);
			stk_unsigned_push(&pause_stk_close, &ntab);
		}
	}

	stk_unsigned_destroy(&pause_stk_open);
	stk_unsigned_destroy(&pause_stk_close);
	
	if (!rds.in_blk_cmt && !rds.in_ln_cmt) {
		size_t i = first_ch;
		while (ntab > 0 && i < mf->buf->size && src[i] == L'}') {
			--ntab;
			++i;
		}
		
		ntab -= ntab > 0 && src[last_sig_ch] == L':';
		if (first_ch != mf->buf->size)
			ntab *= src[first_ch] != L'#';
	}
	
	return ntab;
}

static unsigned
comp_smart_spaces(size_t first_spc, size_t off, size_t ln, size_t first_ch)
{
	wchar_t const *src = mf->buf->conts;
	
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	
	while (first_spc + off < ln
	       && (src[first_spc + off] != L'(' || rds.in_str || rds.in_ch || rds.in_blk_cmt || rds.in_ln_cmt)) {
		if (comp_next_state(first_spc, &off, &rds) == CNS_CONTINUE)
			continue;
		
		++off;
	}
	
	unsigned nopen = 0;
	for (size_t i = first_spc + off; i < ln; ++i) {
		if (comp_next_state(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.in_str || rds.in_ch || rds.in_blk_cmt || rds.in_ln_cmt;
		nopen += src[i] == L'(' && !in;
		nopen -= src[i] == L')' && !in && nopen > 0;
	}
	
	if (first_ch < mf->buf->size
	    && !wcschr(L"{}", src[first_ch])
	    && first_spc + off < ln
	    && nopen > 0) {
		return off + 1;
	}

	return 0;
}

static long
nopen_at(size_t pos, wchar_t open, wchar_t close)
{
	long nopen = 0;
	
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	
	for (size_t i = 0; i < pos; ++i) {
		wchar_t wch = mf->buf->conts[i];
		
		if (comp_next_state(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.in_str || rds.in_ch || rds.in_blk_cmt || rds.in_ln_cmt;
		nopen += !in && wch == open;
		nopen -= !in && wch == close;
	}

	return nopen;
}

static int
comp_next_state(size_t pos, size_t *off, struct rd_state *rds)
{
	wchar_t const *src = mf->buf->conts;
	size_t wch = src[pos + *off];
	
	if (wch == L'\\'
	    && (rds->in_str || rds->in_ch)
	    && !rds->in_blk_cmt
	    && !rds->in_ln_cmt) {
		if (src[pos + ++*off] == L'\n')
			rds->in_str = rds->in_ch = false;
		return CNS_CONTINUE;
	} else if (wch == L'"'
	           && !rds->in_ch
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt) {
		rds->in_str = !rds->in_str;
	} else if (wch == L'\''
	           && !rds->in_str
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt) {
		rds->in_ch = !rds->in_ch;
	} else if (wch == L'\n' && !rds->in_blk_cmt)
		rds->in_ln_cmt = rds->in_str = rds->in_ch = false;
	else if (!wcsncmp(&src[pos + *off], L"//", 2)
	           && !rds->in_ch
	           && !rds->in_str
	           && !rds->in_blk_cmt) {
		++*off;
		rds->in_ln_cmt = true;
	} else if (!wcsncmp(&src[pos + *off], L"/*", 2)
	           && !rds->in_ch
	           && !rds->in_str
	           && !rds->in_ln_cmt) {
		++*off;
		rds->in_blk_cmt = true;
	} else if (!wcsncmp(&src[pos + *off], L"*/", 2) && rds->in_blk_cmt) {
		++*off;
		rds->in_blk_cmt = false;
	}
	
	return 0;
}

static bool
not_iswspace(wchar_t wch)
{
	return !iswspace(wch);
}

static bool
is_sig_last(wchar_t wch)
{
	return wcschr(L"+-()[].<>!~*&/%=?:|,", wch) || iswalnum(wch);
}

static struct vec_mu_region
get_skip_regs(void)
{
	struct vec_mu_region skip = vec_mu_region_create();
	
	wchar_t const *src = mf->buf->conts;
	bool in_str = false, in_ch = false;
	for (size_t i = 0; i < mf->buf->size; ++i) {
		if (src[i] == L'"' && !in_ch)
			in_str = !in_str;
		else if (src[i] == L'\'' && !in_str)
			in_ch = !in_ch;
		else if ((in_ch || in_str) && src[i] == L'\\')
			++i;
		else if (i < mf->buf->size - 1
		         && !in_ch
		         && !in_str
		         && !wcsncmp(&src[i], L"//", 2)) {
			size_t lb = i;
			
			while (i < mf->buf->size && src[i] != L'\n')
				++i;
			
			struct mu_region reg = {
				.lb = lb,
				.ub = --i,
			};
			vec_mu_region_add(&skip, &reg);
		} else if (i < mf->buf->size - 1
		           && !in_ch
		           && !in_str
		           && !wcsncmp(&src[i], L"/*", 2)) {
			size_t lb = i;
			
			while (i < mf->buf->size - 1
			       && wcsncmp(&src[i], L"*/", 2)) {
				++i;
			}
			
			struct mu_region reg = {
				.lb = lb,
				.ub = ++i,
			};
			vec_mu_region_add(&skip, &reg);
		}
	}
	
	return skip;
}
