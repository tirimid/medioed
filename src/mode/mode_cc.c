#include "mode/mode_cc.h"

#include <string.h>
#include <wctype.h>

#include "keybd.h"
#include "mode/mutil.h"
#include "util.h"

#define CNS_CONTINUE 1

struct rd_state {
	// primary state.
	bool in_blk_cmt, in_ln_cmt;
	bool in_ch, in_str, in_rstr;
	
	// secondary state.
	// `in_rstr_cmp` is 19 long to account for `d-char-seq` (max 16 chars),
	// plus 3 for '"', ')', and null.
	wchar_t in_rstr_cmp[19];
	unsigned in_rstr_cmp_len;
};

static void bind_indent(void);
static unsigned comp_tabs(size_t first_ch, size_t last_sig_ch);
static unsigned comp_smart_spaces(size_t first_spc, size_t off, size_t ln, size_t first_ch);
static long nopen_at(size_t pos, wchar_t open, wchar_t close);
static int comp_next_state(size_t pos, size_t *off, struct rd_state *rds);
static bool not_iswspace(wchar_t wch);
static bool is_sig_last(wchar_t wch);
static struct vec_mu_region get_skip_regs(void);

static struct frame *mf;

static int cc_bind_indent[] = {K_TAB, -1};

static wchar_t const *indent_kw[] = {
	L"do",
	L"else",
};

static wchar_t const *pause_kw[] = {
	L"namespace",
};

void
mode_cc_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_set_base();
	mu_set_pairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_ANGLE | PF_SQUOTE | PF_DQUOTE);
	mu_set_bind_new_line(bind_indent);
	
	keybd_bind(cc_bind_indent, bind_indent);
	
	keybd_organize();
}

void
mode_cc_quit(void)
{
}

void
mode_cc_update(void)
{
}

void
mode_cc_keypress(wint_t k)
{
}

static void
bind_indent(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
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
		
		if (buf_get_wch(mf->buf, prev_last_ch) == L')'
		    && buf_get_wch(mf->buf, prev_first_ch) != L'#') {
			if (ntab > 0
			    && nopen_at(prev_last_ch + 1, L'(', L')') == 0
			    && !wcschr(L"{}", buf_get_wch(mf->buf, first_ch))) {
				++ntab;
			}
		} else if (buf_get_wch(mf->buf, prev_last_ch) == L'\\')
			++ntab;

		for (size_t i = 0; i < ARRAY_SIZE(indent_kw); ++i) {
			size_t len = wcslen(indent_kw[i]);
			if (len > prev_last_ch)
				continue;
			
			if (iswalnum(buf_get_wch(mf->buf, prev_last_ch - len)))
				continue;
			
			wchar_t cmp[64];
			buf_get_wstr(mf->buf, cmp, prev_last_ch - len + 1, len + 1);
			if (wcscmp(indent_kw[i], cmp))
				continue;
			
			++ntab;
			break;
		}
		
		unsigned prev_ntab = 0, off = 0;
		size_t first_spc = prev_ln;
		
		while (buf_get_wch(mf->buf, first_spc) == L'\t') {
			++first_spc;
			++prev_ntab;
		}
		while (buf_get_wch(mf->buf, first_spc + off) == L' ')
			++off;
		
		if (first_ch < mf->buf->size
		    && !wcschr(L"{}", buf_get_wch(mf->buf, first_ch))
		    && !wcschr(L";/", buf_get_wch(mf->buf, prev_last_ch))
		    && off != 0
		    && !iswspace(buf_get_wch(mf->buf, first_spc + off))
		    && ntab == prev_ntab) {
			nspace = off;
		} else if (off == 0 && ntab == prev_ntab)
			nspace = comp_smart_spaces(first_spc, off, ln, first_ch);
	}
	
	mu_finish_indent(ln, first_ch, ntab, nspace);
	vec_mu_region_destroy(&skip);
}

static unsigned
comp_tabs(size_t first_ch, size_t last_sig_ch)
{
	unsigned ntab = 0;
	
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	
	struct stk_unsigned pause_stk_open = stk_unsigned_create();
	struct stk_unsigned pause_stk_close = stk_unsigned_create();
	size_t prev_npause = 0;
	
	for (size_t i = 0; i < first_ch; ++i) {
		wchar_t wch = buf_get_wch(mf->buf, i);
		wchar_t cmp_buf[64];
		
		if (comp_next_state(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		if (rds.in_str
		    || rds.in_ch
		    || rds.in_rstr
		    || rds.in_blk_cmt
		    || rds.in_ln_cmt) {
			continue;
		} else if (wcschr(L"\n)", wch)) {
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
		} else if (i + 1 < mf->buf->size
		           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, i, 3), L"::")) {
			++i;
		} else if (wch == L':') {
			stk_unsigned_push(&pause_stk_open, &ntab);
			stk_unsigned_push(&pause_stk_close, &ntab);
		}
		
		for (size_t j = 0; j < ARRAY_SIZE(pause_kw); ++j) {
			if (i > 0 && iswalnum(buf_get_wch(mf->buf, i - 1)))
				continue;
			
			size_t len = wcslen(pause_kw[j]);
			if (i + len >= mf->buf->size
			    || iswalnum(buf_get_wch(mf->buf, i + len))) {
				continue;
			}
			
			buf_get_wstr(mf->buf, cmp_buf, i, len + 1);
			if (!wcscmp(pause_kw[j], cmp_buf)) {
				stk_unsigned_push(&pause_stk_open, &ntab);
				stk_unsigned_push(&pause_stk_close, &ntab);
				i += len - 1;
				break;
			}
		}
	}
	
	stk_unsigned_destroy(&pause_stk_open);
	stk_unsigned_destroy(&pause_stk_close);
	
	if (!rds.in_blk_cmt && !rds.in_ln_cmt) {
		size_t i = first_ch;
		while (ntab > 0
		       && i < mf->buf->size
		       && buf_get_wch(mf->buf, i) == L'}') {
			--ntab;
			++i;
		}
		
		ntab -= ntab > 0 && buf_get_wch(mf->buf, last_sig_ch) == L':';
		if (first_ch != mf->buf->size)
			ntab *= buf_get_wch(mf->buf, first_ch) != L'#';
	}
	
	return ntab;
}

static unsigned
comp_smart_spaces(size_t first_spc, size_t off, size_t ln, size_t first_ch)
{
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	
	while (buf_get_wch(mf->buf, first_spc + off) != L'('
	       || rds.in_str
	       || rds.in_ch
	       || rds.in_rstr
	       || rds.in_blk_cmt
	       || rds.in_ln_cmt) {
		if (first_spc + off >= ln)
			break;
		
		if (comp_next_state(first_spc, &off, &rds) == CNS_CONTINUE)
			continue;
		
		++off;
	}
	
	unsigned nopen = 0;
	for (size_t i = first_spc + off; i < ln; ++i) {
		if (comp_next_state(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.in_str || rds.in_ch || rds.in_rstr || rds.in_blk_cmt || rds.in_ln_cmt;
		wchar_t wch = buf_get_wch(mf->buf, i);
		nopen += wch == L'(' && !in;
		nopen -= wch == L')' && !in && nopen > 0;
	}
	
	if (first_ch < mf->buf->size
	    && !wcschr(L"{}", buf_get_wch(mf->buf, first_ch))
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
		wchar_t wch = buf_get_wch(mf->buf, i);
		
		if (comp_next_state(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.in_str || rds.in_ch || rds.in_rstr || rds.in_blk_cmt || rds.in_ln_cmt;
		nopen += !in && wch == open;
		nopen -= !in && wch == close;
	}

	return nopen;
}

static int
comp_next_state(size_t pos, size_t *off, struct rd_state *rds)
{
	size_t wch = buf_get_wch(mf->buf, pos + *off);
	wchar_t cmp_buf[20]; // max possible read len (19) plus 1.
	
	if (wch == L'\\'
	    && (rds->in_str || rds->in_ch)
	    && !rds->in_rstr
	    && !rds->in_blk_cmt
	    && !rds->in_ln_cmt) {
		if (buf_get_wch(mf->buf, pos + ++*off) == L'\n')
			rds->in_str = rds->in_ch = false;
		return CNS_CONTINUE;
	} else if (pos + *off + 2 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 3), L"R\"")
	           || pos + *off + 3 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 4), L"LR\"")
	           || pos + *off + 4 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 5), L"u8R\"")
	           || pos + *off + 3 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 4), L"uR\"")
	           || pos + *off + 3 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 4), L"UR\"")) {
		if (rds->in_str
		    || rds->in_ch
		    || rds->in_rstr
		    || rds->in_blk_cmt
		    || rds->in_ln_cmt) {
			return CNS_CONTINUE;
		}
		
		while (buf_get_wch(mf->buf, pos + *off) != L'"')
			++*off;
		
		size_t ub = pos + *off + 1;
		while (ub < mf->buf->size
		       && ub - pos - *off <= 16
		       && !wcschr(L"(\"", buf_get_wch(mf->buf, ub))) {
			++ub;
		}
		
		if (ub >= mf->buf->size || buf_get_wch(mf->buf, ub) != L'(')
			return CNS_CONTINUE;
		
		size_t d_char_seq_len = ub - pos - *off - 1;
		rds->in_rstr_cmp_len = d_char_seq_len + 2;
		
		for (size_t i = 0; i < d_char_seq_len; ++i)
			rds->in_rstr_cmp[i + 1] = buf_get_wch(mf->buf, pos + *off + i + 1);
		rds->in_rstr_cmp[0] = L')';
		rds->in_rstr_cmp[d_char_seq_len + 1] = L'"';
		rds->in_rstr_cmp[d_char_seq_len + 2] = 0;
		
		rds->in_rstr = true;
	} else if (pos + *off + rds->in_rstr_cmp_len - 1 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, rds->in_rstr_cmp_len + 1), rds->in_rstr_cmp)
	           && rds->in_rstr) {
		*off += rds->in_rstr_cmp_len - 1;
		rds->in_rstr = false;
	} else if (wch == L'"'
	           && !rds->in_ch
	           && !rds->in_rstr
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt) {
		rds->in_str = !rds->in_str;
	} else if (wch == L'\''
	           && !rds->in_str
	           && !rds->in_rstr
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt) {
		rds->in_ch = !rds->in_ch;
	} else if (wch == L'\n' && !rds->in_blk_cmt)
		rds->in_ln_cmt = rds->in_str = rds->in_ch = false;
	else if (pos + *off + 1 < mf->buf->size
	         && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 3), L"//")
	         && !rds->in_ch
	         && !rds->in_str
	         && !rds->in_rstr
	         && !rds->in_blk_cmt) {
		++*off;
		rds->in_ln_cmt = true;
	} else if (pos + *off + 1 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 3), L"/*")
	           && !rds->in_ch
	           && !rds->in_str
	           && !rds->in_rstr
	           && !rds->in_ln_cmt) {
		++*off;
		rds->in_blk_cmt = true;
	} else if (pos + *off + 1 < mf->buf->size
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, pos + *off, 3), L"*/")
	           && rds->in_blk_cmt) {
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
	
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	
	bool in_cmt = false;
	size_t lb = 0;
	for (size_t i = 0; i < mf->buf->size; ++i) {
		if (comp_next_state(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		if (!in_cmt && (rds.in_ln_cmt || rds.in_blk_cmt)) {
			lb = i - 1;
			in_cmt = true;
		} else if (in_cmt && !rds.in_ln_cmt && !rds.in_blk_cmt) {
			in_cmt = false;
			struct mu_region reg = {
				.lb = lb,
				.ub = i,
			};
			vec_mu_region_add(&skip, &reg);
		}
	}
	
	return skip;
}
