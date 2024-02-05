#include "mode/mode_rs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "conf.h"
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
static void bind_new_line(void);
static long nopen_at(struct rd_state *out_rds, size_t pos, wchar_t const *open, wchar_t const *close);
static int comp_next_state(size_t *off, size_t limit, struct rd_state *rds);

static struct frame *mf;

static int rs_bind_indent[] = {K_TAB, -1};

void
mode_rs_init(struct frame *f)
{
	mf = f;
	mu_init(f);

	mu_set_base();
	mu_set_pairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_ANGLE | PF_DQUOTE);

	keybd_bind(conf_bind_new_line, bind_new_line);
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
	
	wchar_t const *src = mf->buf->conts;
	
	size_t ln = mf->csr;
	while (ln > 0 && src[ln - 1] != L'\n')
		--ln;

	size_t prev_ln = ln - (ln > 0);
	while (prev_ln > 0 && src[prev_ln - 1] != L'\n')
		--prev_ln;

	size_t first_ch = ln;
	while (first_ch < mf->buf->size
	       && src[first_ch] != L'\n'
	       && iswspace(src[first_ch])) {
		++first_ch;
	}
	
	struct rd_state rds;
	long ntab = nopen_at(&rds, first_ch, L"([{", L")]}");
	
	if (!rds.in_blk_cmt && !rds.in_ln_cmt && prev_ln != ln) {
		size_t prev_last_ch = ln - 1;
		while (prev_last_ch > prev_ln && iswspace(src[prev_last_ch]))
			--prev_last_ch;
		
		size_t prev_first_ch = prev_ln;
		while (iswspace(src[prev_first_ch]))
			++prev_first_ch;
		
		if (!iswspace(src[prev_last_ch])
		    && !iswspace(src[first_ch])
		    && !wcschr(L"([{};,", src[prev_last_ch])
		    && !wcschr(L")]}", src[first_ch])
		    && !wcschr(L"#", src[prev_first_ch])
		    && wcsncmp(L"//", &src[prev_first_ch], 2)
		    && wcsncmp(L"//", &src[first_ch], 2)
		    && (prev_last_ch == 0 || wcsncmp(L"*/", &src[prev_last_ch - 1], 2))) {
			++ntab;
		}
	}
	
	if (!rds.in_blk_cmt && !rds.in_ln_cmt) {
		size_t i = first_ch;
		while (ntab > 0
		       && i < mf->buf->size
		       && wcschr(L")]}", src[i])) {
			--ntab;
			++i;
		}
	}
	
	ntab = MAX(0, ntab);
	
	mu_finish_indent(ln, first_ch, ntab, 0);
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

static long
nopen_at(struct rd_state *out_rds, size_t pos, wchar_t const *open,
         wchar_t const *close)
{
	wchar_t const *src = mf->buf->conts;
	
	struct rd_state rds;
	memset(&rds, 0, sizeof(rds));
	rds.in_rstr_cmp = malloc(1);
	
	long nopen = 0;
	for (size_t i = 0; i < pos; ++i) {
		if (comp_next_state(&i, pos, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.in_rstr || rds.in_str || rds.in_ch || rds.in_blk_cmt || rds.in_ln_cmt;
		nopen += !in && wcschr(open, src[i]);
		nopen -= !in && wcschr(close, src[i]);
	}
	
	free(rds.in_rstr_cmp);
	
	if (out_rds)
		memcpy(out_rds, &rds, sizeof(rds));
	return nopen;
}

static int
comp_next_state(size_t *off, size_t limit, struct rd_state *rds)
{
	wchar_t const *src = mf->buf->conts;
	
	if (!rds->in_rstr
	    && !rds->in_str
	    && !rds->in_ch
	    && !rds->in_blk_cmt
	    && !rds->in_ln_cmt
	    && *off + 1 < limit
	    && src[*off] == L'r'
	    && wcschr(L"#\"", src[*off + 1])) {
		size_t i = *off + 1;
		while (i < limit && src[i] == L'#')
			++i;
		
		if (i >= limit || src[i] != L'"')
			return CNS_CONTINUE;
		
		i += i < limit;
		rds->in_rstr_cmp_len = i - *off - 1;
		rds->in_rstr_cmp = realloc(rds->in_rstr_cmp, sizeof(wchar_t) * rds->in_rstr_cmp_len);
		rds->in_rstr_cmp[0] = L'"';
		
		for (unsigned j = 1; j < rds->in_rstr_cmp_len; ++j)
			rds->in_rstr_cmp[j] = L'#';
		
		rds->in_rstr = true;
	} else if (!rds->in_rstr
	           && !rds->in_ch
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt
	           && src[*off] == L'"') {
		rds->in_str = !rds->in_str;
	} else if (!rds->in_rstr
	           && !rds->in_str
	           && !rds->in_blk_cmt
	           && !rds->in_ln_cmt
	           && src[*off] == L'\'') {
		rds->in_ch = !rds->in_ch;
		if (rds->in_ch) {
			rds->in_ch_esc = *off + 1 < mf->buf->size && src[*off + 1] == L'\\';
			rds->in_ch_nch = 0;
		}
	} else if (rds->in_ch && !rds->in_ch_esc && rds->in_ch_nch > 1)
		rds->in_ch = false;
	else if (rds->in_rstr
	         && *off + rds->in_rstr_cmp_len < limit
	         && !wcsncmp(&src[*off], rds->in_rstr_cmp, rds->in_rstr_cmp_len)) {
		rds->in_rstr = false;
		*off += rds->in_rstr_cmp_len;
	} else if ((rds->in_str || rds->in_ch) && src[*off] == L'\\')
		++*off;
	else if (!rds->in_rstr
	         && !rds->in_str
	         && !rds->in_ch
	         && !rds->in_blk_cmt
	         && *off + 1 < limit
	         && !wcsncmp(&src[*off], L"//", 2)) {
		++*off;
		rds->in_ln_cmt = true;
	} else if (rds->in_ln_cmt && src[*off] == L'\n')
		rds->in_ln_cmt = false;
	else if (!rds->in_rstr
	         && !rds->in_str
	         && !rds->in_ch
	         && !rds->in_ln_cmt
	         && *off + 1 < limit
	         && !wcsncmp(&src[*off], L"/*", 2)) {
		++*off;
		rds->in_blk_cmt = true;
		++rds->in_blk_cmt_ncmt;
	} else if (rds->in_blk_cmt
	           && *off + 1 < limit
	           && !wcsncmp(&src[*off], L"*/", 2)) {
		++*off;
		if (--rds->in_blk_cmt_ncmt == 0)
			rds->in_blk_cmt = false;
	}
	
	rds->in_ch_nch += rds->in_ch;
	return 0;
}
