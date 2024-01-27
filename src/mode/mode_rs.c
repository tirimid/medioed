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

#define CNS_CONTINUE 2

struct rdstate {
	// primary state.
	bool inblkcmt, inlncmt;
	bool inch, instr, inrstr;
	
	// secondary state.
	bool inch_esc;
	size_t inch_nch, inblkcmt_ncmt;
	wchar_t *inrstr_cmp;
	unsigned inrstr_cmplen;
};

static void bind_indent(void);
static void bind_newline(void);
static long nopenat(struct rdstate *out_rds, size_t pos, wchar_t const *open, wchar_t const *close);
static int compnextstate(size_t pos, size_t *off, size_t limit, struct rdstate *rds);

static struct frame *mf;

static int rs_bind_indent[] = {K_TAB, -1};

void
mode_rs_init(struct frame *f)
{
	mf = f;
	mu_init(f);

	mu_setbase();
	mu_setpairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_ANGLE | PF_DQUOTE);

	keybd_bind(conf_bind_newline, bind_newline);
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
	
	// figure out indentation parameters.
	size_t ln = mf->csr;
	while (ln > 0 && src[ln - 1] != L'\n')
		--ln;

	size_t prevln = ln - (ln > 0);
	while (prevln > 0 && src[prevln - 1] != L'\n')
		--prevln;

	size_t firstch = ln;
	while (firstch < mf->buf->size
	       && src[firstch] != L'\n'
	       && iswspace(src[firstch])) {
		++firstch;
	}
	
	struct rdstate rds;
	long ntab = nopenat(&rds, firstch, L"([{", L")]}");
	
	if (!rds.inblkcmt && !rds.inlncmt && prevln != ln) {
		size_t prevlastch = ln - 1;
		while (prevlastch > prevln && iswspace(src[prevlastch]))
			--prevlastch;
		
		size_t prevfirstch = prevln;
		while (iswspace(src[prevfirstch]))
			++prevfirstch;
		
		if (!iswspace(src[prevlastch])
		    && !iswspace(src[firstch])
		    && !wcschr(L"([{};,", src[prevlastch])
		    && !wcschr(L")]}", src[firstch])
		    && !wcschr(L"#", src[prevfirstch])
		    && wcsncmp(L"//", &src[prevfirstch], 2)
		    && wcsncmp(L"//", &src[firstch], 2)
		    && (prevlastch == 0 || wcsncmp(L"*/", &src[prevlastch - 1], 2))) {
			++ntab;
		}
	}
	
	if (!rds.inblkcmt && !rds.inlncmt) {
		size_t i = firstch;
		while (ntab > 0
		       && i < mf->buf->size
		       && wcschr(L")]}", src[i])) {
			--ntab;
			++i;
		}
		
		ntab = MAX(0, ntab);
	}
	
	// do indentation.
	buf_erase(mf->buf, ln, firstch);
	for (unsigned i = 0; i < ntab; ++i)
		buf_writewch(mf->buf, ln + i, L'\t');
	
	// fix cursor.
	if (mf->csr <= firstch)
		mf->csr = ln;
	else {
		mf->csr -= firstch - ln;
		mf->csr_wantcol -= firstch - ln;
	}
	frame_relmvcsr(mf, 0, ntab, false);
}

static void
bind_newline(void)
{
	bind_indent();

	buf_pushhistbrk(mf->buf);
	buf_writewch(mf->buf, mf->csr, L'\n');
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
	
	bind_indent();
}

static long
nopenat(struct rdstate *out_rds, size_t pos, wchar_t const *open,
        wchar_t const *close)
{
	long nopen = 0;
	wchar_t const *src = mf->buf->conts;
	struct rdstate rds = {
		.inrstr_cmp = malloc(1),
	};
	
	for (size_t i = 0; i < pos; ++i) {
		if (compnextstate(0, &i, pos, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.inch || rds.instr || rds.inrstr || rds.inblkcmt || rds.inlncmt;
		nopen += !in && wcschr(open, src[i]);
		nopen -= !in && wcschr(close, src[i]);
	}
	
	free(rds.inrstr_cmp);
	
	if (out_rds)
		*out_rds = rds;
	return nopen;
}

static int
compnextstate(size_t pos, size_t *off, size_t limit, struct rdstate *rds)
{
	wchar_t const *src = mf->buf->conts;
	
	if (!rds->inrstr
	    && !rds->instr
	    && !rds->inch
	    && !rds->inblkcmt
	    && !rds->inlncmt
	    && pos + *off + 1 < limit
	    && src[pos + *off] == L'r'
	    && wcschr(L"#\"", src[pos + *off + 1])) {
		size_t i = pos + *off + 1;
		while (i < limit && src[i] == L'#')
			++i;
		
		if (i >= limit || src[i] != L'"')
			return CNS_CONTINUE;
		
		i += i < limit;
		rds->inrstr_cmplen = i - pos - *off - 1;
		rds->inrstr_cmp = realloc(rds->inrstr_cmp, sizeof(wchar_t) * rds->inrstr_cmplen);
		rds->inrstr_cmp[0] = L'"';
		
		for (unsigned j = 1; j < rds->inrstr_cmplen; ++j)
			rds->inrstr_cmp[j] = L'#';
		
		rds->inrstr = true;
		goto done;
	} else if (!rds->inrstr
	           && !rds->inch
	           && !rds->inblkcmt
	           && !rds->inlncmt
	           && src[pos + *off] == L'"') {
		rds->instr = !rds->instr;
		goto done;
	} else if (!rds->inrstr
	           && !rds->instr
	           && !rds->inblkcmt
	           && !rds->inlncmt
	           && src[pos + *off] == L'\'') {
		rds->inch = !rds->inch;
		if (rds->inch) {
			rds->inch_esc = pos + *off + 1 < mf->buf->size && src[pos + *off + 1] == L'\\';
			rds->inch_nch = 0;
		}
		goto done;
	} else if (rds->inch && !rds->inch_esc && rds->inch_nch > 1) {
		rds->inch = false;
		goto done;
	} else if (rds->inrstr
	           && pos + *off + rds->inrstr_cmplen < limit
	           && !wcsncmp(&src[pos + *off], rds->inrstr_cmp, rds->inrstr_cmplen)) {
		rds->inrstr = false;
		*off += rds->inrstr_cmplen;
		goto done;
	} else if ((rds->instr || rds->inch) && src[pos + *off] == L'\\') {
		++*off;
		return CNS_CONTINUE;
	} else if (!rds->inrstr
	           && !rds->instr
	           && !rds->inch
	           && !rds->inblkcmt
	           && pos + *off + 1 < limit
	           && !wcsncmp(&src[pos + *off], L"//", 2)) {
		++*off;
		rds->inlncmt = true;
		goto done;
	} else if (rds->inlncmt && src[pos + *off] == L'\n') {
		rds->inlncmt = false;
		goto done;
	} else if (!rds->inrstr
	           && !rds->instr
	           && !rds->inch
	           && !rds->inlncmt
	           && pos + *off + 1 < limit
	           && !wcsncmp(&src[pos + *off], L"/*", 2)) {
		++*off;
		rds->inblkcmt = true;
		++rds->inblkcmt_ncmt;
		goto done;
	} else if (rds->inblkcmt
	           && pos + *off + 1 < limit
	           && !wcsncmp(&src[pos + *off], L"*/", 2)) {
		++*off;
		if (--rds->inblkcmt_ncmt == 0)
			rds->inblkcmt = false;
		goto done;
	}
	
	return 1;
	
done:
	rds->inch_nch += rds->inch;
	return 0;
}
