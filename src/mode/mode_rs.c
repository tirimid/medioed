#include "mode/mode_rs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
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
	bool in_ch_esc;
	size_t in_ch_nch, in_blk_cmt_ncmt;
	wchar_t *in_rstr_cmp;
	unsigned in_rstr_cmp_len;
};

static void bind_indent(void);
static long nopen_at(struct rd_state *out_rds, size_t pos, wchar_t const *open, wchar_t const *close);
static int comp_next_state(size_t *off, size_t limit, struct rd_state *rds);

static struct frame *mf;

static int rs_bind_indent[] = {K_TAB, -1};

static wchar_t const *no_indent_kw[] = {
	L"where",
};

void
mode_rs_init(struct frame *f)
{
	mf = f;
	mu_init(f);

	mu_set_base();
	mu_set_pairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_ANGLE | PF_DQUOTE);
	mu_set_bind_new_line(bind_indent);

	keybd_bind(rs_bind_indent, bind_indent);

	keybd_organize();
}

void
mode_rs_quit(void)
{
}

void
mode_rs_update(void)
{
}

void
mode_rs_keypress(wint_t k)
{
}

static void
bind_indent(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
	// TODO: rework to use skip-region technique.
	
	size_t ln = mf->csr;
	while (ln > 0 && buf_get_wch(mf->buf, ln - 1) != L'\n')
		--ln;

	size_t prev_ln = ln - (ln > 0);
	while (prev_ln > 0 && buf_get_wch(mf->buf, prev_ln - 1) != L'\n')
		--prev_ln;

	size_t first_ch = ln;
	while (first_ch < mf->buf->size
	       && buf_get_wch(mf->buf, first_ch) != L'\n'
	       && iswspace(buf_get_wch(mf->buf, first_ch))) {
		++first_ch;
	}
	
	struct rd_state rds;
	long ntab = nopen_at(&rds, first_ch, L"([{", L")]}");
	
	if (!rds.in_blk_cmt && !rds.in_ln_cmt && prev_ln != ln) {
		size_t prev_last_ch = ln - 1;
		while (prev_last_ch > prev_ln
		       && iswspace(buf_get_wch(mf->buf, prev_last_ch))) {
			--prev_last_ch;
		}
		
		size_t prev_first_ch = prev_ln;
		while (prev_first_ch < mf->buf->size
		       && iswspace(buf_get_wch(mf->buf, prev_last_ch))) {
			++prev_first_ch;
		}
		
		size_t last_ch = first_ch;
		while (last_ch < mf->buf->size
		       && buf_get_wch(mf->buf, last_ch) != L'\n') {
			++last_ch;
		}
		while (last_ch >= ln && iswspace(buf_get_wch(mf->buf, last_ch)))
			--last_ch;
		
		bool no_indent = false;
		for (size_t i = 0; i < ARRAY_SIZE(no_indent_kw); ++i) {
			size_t len = wcslen(no_indent_kw[i]);
			if (len > last_ch)
				continue;
			
			if (iswalnum(buf_get_wch(mf->buf, last_ch - len)))
				continue;
			
			wchar_t cmp[64];
			buf_get_wstr(mf->buf, cmp, last_ch - len + 1, len);
			if (wcscmp(no_indent_kw[i], cmp))
				continue;
			
			no_indent = true;
			break;
		}
		
		// TODO: add `where` clause handling.
		
		wchar_t cmp_buf[3];
		if (!iswspace(buf_get_wch(mf->buf, prev_last_ch))
		    && !iswspace(buf_get_wch(mf->buf, first_ch))
		    && !wcschr(L"([{};,", buf_get_wch(mf->buf, prev_last_ch))
		    && !wcschr(L")]}", buf_get_wch(mf->buf, first_ch))
		    && !wcschr(L"#", buf_get_wch(mf->buf, prev_first_ch))
		    && wcscmp(L"//", buf_get_wstr(mf->buf, cmp_buf, prev_first_ch, 2))
		    && wcscmp(L"//", buf_get_wstr(mf->buf, cmp_buf, first_ch, 2))
		    && (prev_last_ch == 0 || wcscmp(L"*/", buf_get_wstr(mf->buf, cmp_buf, prev_last_ch - 1, 2)))
		    && !no_indent) {
			++ntab;
		}
	}
	
	if (!rds.in_blk_cmt && !rds.in_ln_cmt) {
		size_t i = first_ch;
		while (ntab > 0
		       && i < mf->buf->size
		       && wcschr(L")]}", buf_get_wch(mf->buf, i))) {
			--ntab;
			++i;
		}
	}
	
	ntab = MAX(0, ntab);
	
	mu_finish_indent(ln, first_ch, ntab, 0);
}

static long
nopen_at(struct rd_state *out_rds, size_t pos, wchar_t const *open,
         wchar_t const *close)
{
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	rds.in_rstr_cmp = malloc(1);
	
	long nopen = 0;
	for (size_t i = 0; i < pos; ++i) {
		if (comp_next_state(&i, pos, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.in_rstr || rds.in_str || rds.in_ch || rds.in_blk_cmt || rds.in_ln_cmt;
		wchar_t wch = buf_get_wch(mf->buf, i);
		nopen += !in && wcschr(open, wch);
		nopen -= !in && wcschr(close, wch);
	}
	
	free(rds.in_rstr_cmp);
	
	if (out_rds)
		memcpy(out_rds, &rds, sizeof(rds));
	return nopen;
}

static int
comp_next_state(size_t *off, size_t limit, struct rd_state *rds)
{
	// this will probably segfault for `in_rstr_cmp_len` >= 64.
	// but, then again, nobody ever writes a 64-level raw string in Rust.
	// TODO: make this not crash.
	wchar_t cmp_buf[64];
	
	if (!rds->in_rstr
	    && !rds->in_str
	    && !rds->in_ch
	    && !rds->in_blk_cmt
	    && !rds->in_ln_cmt
	    && *off + 1 < limit
	    && buf_get_wch(mf->buf, *off) == L'r'
	    && wcschr(L"#\"", buf_get_wch(mf->buf, *off + 1))) {
		size_t i = *off + 1;
		while (i < limit && buf_get_wch(mf->buf, i) == L'#')
			++i;
		
		if (i >= limit || buf_get_wch(mf->buf, i) != L'"')
			return CNS_CONTINUE;
		
		i += i < limit;
		rds->in_rstr_cmp_len = i - *off - 1;
		rds->in_rstr_cmp = realloc(rds->in_rstr_cmp, sizeof(wchar_t) * (rds->in_rstr_cmp_len + 1));
		rds->in_rstr_cmp[0] = L'"';
		rds->in_rstr_cmp[rds->in_rstr_cmp_len] = 0;
		
		for (unsigned j = 1; j < rds->in_rstr_cmp_len; ++j)
			rds->in_rstr_cmp[j] = L'#';
		
		rds->in_rstr = true;
	} else if (!rds->in_rstr
	           && !rds->in_ch
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt
	           && buf_get_wch(mf->buf, *off) == L'"') {
		rds->in_str = !rds->in_str;
	} else if (!rds->in_rstr
	           && !rds->in_str
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt
	           && buf_get_wch(mf->buf, *off) == L'\'') {
		rds->in_ch = !rds->in_ch;
		if (rds->in_ch) {
			rds->in_ch_esc = *off + 1 < mf->buf->size;
			rds->in_ch_esc += buf_get_wch(mf->buf, *off + 1) == L'\\';
			rds->in_ch_nch = 0;
		}
	} else if (rds->in_ch && !rds->in_ch_esc && rds->in_ch_nch > 1)
		rds->in_ch = false;
	else if (rds->in_rstr
	         && *off + rds->in_rstr_cmp_len < limit
	         && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, *off, rds->in_rstr_cmp_len), rds->in_rstr_cmp)) {
		rds->in_rstr = false;
		*off += rds->in_rstr_cmp_len;
	} else if ((rds->in_str || rds->in_ch)
	           && buf_get_wch(mf->buf, *off) == L'\\') {
		++*off;
	} else if (!rds->in_rstr
	           && !rds->in_str
	           && !rds->in_ch
	           && !rds->in_blk_cmt
	           && *off + 1 < limit
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, *off, 2), L"//")) {
		++*off;
		rds->in_ln_cmt = true;
	} else if (rds->in_ln_cmt && buf_get_wch(mf->buf, *off) == L'\n')
		rds->in_ln_cmt = false;
	else if (!rds->in_rstr
	         && !rds->in_str
	         && !rds->in_ch
	         && !rds->in_ln_cmt
	         && *off + 1 < limit
	         && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, *off, 2), L"/*")) {
		++*off;
		rds->in_blk_cmt = true;
		++rds->in_blk_cmt_ncmt;
	} else if (rds->in_blk_cmt
	           && *off + 1 < limit
	           && !wcscmp(buf_get_wstr(mf->buf, cmp_buf, *off, 2), L"*/")) {
		++*off;
		if (--rds->in_blk_cmt_ncmt == 0)
			rds->in_blk_cmt = false;
	}
	
	rds->in_ch_nch += rds->in_ch;
	return 0;
}
