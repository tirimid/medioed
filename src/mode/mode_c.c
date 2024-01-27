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

struct rdstate {
	bool inblkcmt, inlncmt;
	bool inch, instr;
};

static void bind_indent(void);
static void bind_newline(void);
static unsigned comptabs(size_t firstch, size_t lastsigch);
static unsigned compsmartspaces(size_t firstspc, size_t off, size_t ln, size_t firstch);
static long nopenat(size_t pos, wchar_t open, wchar_t close);
static int compnextstate(size_t pos, size_t *off, struct rdstate *rds);

static struct frame *mf;

static int c_bind_indent[] = {K_TAB, -1};

static wchar_t const *indentkw[] = {
	L"do",
	L"else",
};

void
mode_c_init(struct frame *f)
{
	mf = f;
	mu_init(f);

	mu_setbase();
	mu_setpairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_SQUOTE | PF_DQUOTE);

	keybd_bind(c_bind_indent, bind_indent);
	keybd_bind(conf_bind_newline, bind_newline);
	
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

	// figure out identation parameters.
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
	
	size_t lastsigch = ln;
	while (lastsigch < mf->buf->size && src[lastsigch] != L'\n')
		++lastsigch;
	while (lastsigch > ln
	       && lastsigch < mf->buf->size
	       && !wcschr(L":'\"", src[lastsigch])) {
		--lastsigch;
	}
	
	unsigned ntab = comptabs(firstch, lastsigch), nspace = 0;
	
	if (prevln != ln) {
		size_t prevlastch = ln - 2 * (ln > 1);
		while (prevlastch > prevln && iswspace(src[prevlastch]))
			--prevlastch;

		size_t prevfirstch = prevln;
		while (prevfirstch < ln - 1 && iswspace(src[prevfirstch]))
			++prevfirstch;
		
		if (src[prevlastch] == L')' && src[prevfirstch] != L'#') {
			if (nopenat(prevlastch + 1, L'(', L')') == 0
			    && !wcschr(L"{}", src[firstch])) {
				++ntab;
			}
		} else if (src[prevlastch] == L'\\')
			++ntab;

		for (size_t i = 0; i < ARRAYSIZE(indentkw); ++i) {
			size_t len = wcslen(indentkw[i]);
			
			if (len <= prevlastch
			    && !iswalnum(src[prevlastch - len])
			    && !wcsncmp(indentkw[i], &src[prevlastch - len + 1], len)) {
				++ntab;
				break;
			}
		}
		
		unsigned prevntab = 0, off = 0;
		size_t firstspc = prevln;
		
		while (src[firstspc] == L'\t') {
			++firstspc;
			++prevntab;
		}
		while (src[firstspc + off] == L' ')
			++off;
		
		if (firstch < mf->buf->size
		    && !wcschr(L"{}", src[firstch])
		    && !wcschr(L";/", src[prevlastch])
		    && off != 0
		    && !iswspace(src[firstspc + off])
		    && ntab == prevntab) {
			nspace = off;
		} else if (off == 0 && ntab == prevntab)
			nspace = compsmartspaces(firstspc, off, ln, firstch);
	}
	
	// do indentation.
	buf_erase(mf->buf, ln, firstch);
	for (unsigned i = 0; i < ntab; ++i)
		buf_writewch(mf->buf, ln + i, L'\t');
	for (unsigned i = 0; i < nspace; ++i)
		buf_writewch(mf->buf, ln + ntab + i, L' ');

	// fix cursor.
	if (mf->csr <= firstch)
		mf->csr = ln;
	else {
		mf->csr -= firstch - ln;
		mf->csr_wantcol -= firstch - ln;
	}
	frame_relmvcsr(mf, 0, ntab + nspace, false);
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

static unsigned
comptabs(size_t firstch, size_t lastsigch)
{
	wchar_t const *src = mf->buf->conts;
	
	unsigned ntab = 0;
	
	struct rdstate rds;
	memset(&rds, 0, sizeof(rds));
	
	struct stk_unsigned pstk_open = stk_unsigned_create();
	struct stk_unsigned pstk_close = stk_unsigned_create();
	size_t prevnpause = 0;
	
	for (size_t i = 0; i < firstch; ++i) {
		wchar_t wch = src[i];
		
		if (compnextstate(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		if (rds.instr || rds.inch || rds.inblkcmt || rds.inlncmt)
			continue;
		else if (wch == L'\n') {
			if (pstk_open.size > prevnpause) {
				free(stk_unsigned_pop(&pstk_open));
				free(stk_unsigned_pop(&pstk_close));
			}
		} else if (wch == L'{') {
			if (pstk_open.size > 0
			    && *stk_unsigned_peek(&pstk_open) == ntab) {
				free(stk_unsigned_pop(&pstk_open));
			} else
				++ntab;
		} else if (wch == L'}' && ntab > 0) {
			if (pstk_close.size > 0
			    && *stk_unsigned_peek(&pstk_close) == ntab) {
				free(stk_unsigned_pop(&pstk_close));
			} else
				ntab -= ntab > 0;
		} else if (wch == L':') {
			stk_unsigned_push(&pstk_open, &ntab);
			stk_unsigned_push(&pstk_close, &ntab);
		}
	}

	stk_unsigned_destroy(&pstk_open);
	stk_unsigned_destroy(&pstk_close);
	
	if (!rds.inblkcmt && !rds.inlncmt) {
		size_t i = firstch;
		while (ntab > 0 && i < mf->buf->size && src[i] == L'}') {
			--ntab;
			++i;
		}
		
		ntab -= ntab > 0 && src[lastsigch] == L':';
		if (firstch != mf->buf->size)
			ntab *= src[firstch] != L'#';
	}
	
	return ntab;
}

static unsigned
compsmartspaces(size_t firstspc, size_t off, size_t ln, size_t firstch)
{
	wchar_t const *src = mf->buf->conts;
	
	struct rdstate rds;
	memset(&rds, 0, sizeof(rds));
	
	while (firstspc + off < ln
	       && (src[firstspc + off] != L'(' || rds.instr || rds.inch || rds.inblkcmt || rds.inlncmt)) {
		if (compnextstate(firstspc, &off, &rds) == CNS_CONTINUE)
			continue;
		
		++off;
	}
	
	unsigned nopen = 0;
	for (size_t i = firstspc + off; i < ln; ++i) {
		if (compnextstate(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.instr || rds.inch || rds.inblkcmt || rds.inlncmt;
		nopen += src[i] == L'(' && !in;
		nopen -= src[i] == L')' && !in && nopen > 0;
	}
	
	if (firstch < mf->buf->size
	    && !wcschr(L"{}", src[firstch])
	    && firstspc + off < ln
	    && nopen > 0) {
		return off + 1;
	}

	return 0;
}

static long
nopenat(size_t pos, wchar_t open, wchar_t close)
{
	long nopen = 0;
	
	struct rdstate rds;
	memset(&rds, 0, sizeof(rds));
	
	for (size_t i = 0; i < pos; ++i) {
		wchar_t wch = mf->buf->conts[i];
		
		if (compnextstate(0, &i, &rds) == CNS_CONTINUE)
			continue;
		
		bool in = rds.instr || rds.inch || rds.inblkcmt || rds.inlncmt;
		nopen += !in && wch == open;
		nopen -= !in && wch == close;
	}

	return nopen;
}

static int
compnextstate(size_t pos, size_t *off, struct rdstate *rds)
{
	wchar_t const *src = mf->buf->conts;
	size_t wch = src[pos + *off];
	
	if (wch == L'\\'
	    && (rds->instr || rds->inch)
	    && !rds->inblkcmt
	    && !rds->inlncmt) {
		if (src[pos + ++*off] == L'\n')
			rds->instr = rds->inch = false;
		return CNS_CONTINUE;
	} else if (wch == L'"'
	           && !rds->inch
	           && !rds->inblkcmt
	           && !rds->inlncmt) {
		rds->instr = !rds->instr;
	} else if (wch == L'\''
	           && !rds->instr
	           && !rds->inblkcmt
	           && !rds->inlncmt) {
		rds->inch = !rds->inch;
	} else if (wch == L'\n' && !rds->inblkcmt)
		rds->inlncmt = rds->instr = rds->inch = false;
	else if (!wcsncmp(&src[pos + *off], L"//", 2)
	           && !rds->inch
	           && !rds->instr
	           && !rds->inblkcmt) {
		++*off;
		rds->inlncmt = true;
	} else if (!wcsncmp(&src[pos + *off], L"/*", 2)
	           && !rds->inch
	           && !rds->instr
	           && !rds->inlncmt) {
		++*off;
		rds->inblkcmt = true;
	} else if (!wcsncmp(&src[pos + *off], L"*/", 2) && rds->inblkcmt) {
		++*off;
		rds->inblkcmt = false;
	}
	
	return 0;
}
