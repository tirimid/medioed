#include "mode/mode_c.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"
#include "mode/mutil.h"
#include "util.h"

static void bind_indent(void);
static void bind_newline(void);
static unsigned comptabs(size_t firstch, size_t lastsigch);
static unsigned compsmartspaces(size_t firstspc, size_t off, size_t ln, size_t firstch);
static long nopenat(size_t pos, wchar_t open, wchar_t close);

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
	       && !iswalnum(src[lastsigch])
	       && !wcschr(L":;", src[lastsigch])) {
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
	
	bool inblkcmt = false, inlncmt = false;
	bool instr = false, inch = false;
	struct stk_unsigned pstk_open = stk_unsigned_create();
	struct stk_unsigned pstk_close = stk_unsigned_create();
	size_t prevnpause = 0;
	
	for (size_t i = 0; i < firstch; ++i) {
		wchar_t wch = src[i];

		switch (wch) {
		case L'\\':
			if ((instr || inch)
			    && mf->buf->conts[++i] == L'\n'
			    && !inblkcmt
			    && !inlncmt) {
				instr = inch = false;
			}
			break;
		case L'"':
			if (!inch && !inblkcmt && !inlncmt)
				instr = !instr;
			break;
		case L'\'':
			if (!instr && !inblkcmt && !inlncmt)
				inch = !inch;
			break;
		case L'\n':
			if (inblkcmt)
				break;
			inlncmt = instr = inch = false;
			if (pstk_open.size > prevnpause) {
				free(stk_unsigned_pop(&pstk_open));
				free(stk_unsigned_pop(&pstk_close));
			}
			break;
		case L'{':
			if (instr || inch || inblkcmt || inlncmt)
				break;
			if (pstk_open.size > 0
			    && *stk_unsigned_peek(&pstk_open) == ntab) {
				free(stk_unsigned_pop(&pstk_open));
			} else
				++ntab;
			break;
		case L'}':
			if (instr || inch || inblkcmt || inlncmt || ntab == 0)
				break;
			if (pstk_close.size > 0
			    && *stk_unsigned_peek(&pstk_close) == ntab) {
				free(stk_unsigned_pop(&pstk_close));
			} else
				ntab -= ntab > 0;
			break;
		case L':':
			if (instr || inch || inblkcmt || inlncmt)
				break;
			stk_unsigned_push(&pstk_open, &ntab);
			stk_unsigned_push(&pstk_close, &ntab);
			break;
		case L'/':
			if (instr
			    || inch
			    || inblkcmt
			    || inlncmt
			    || i + 1 >= firstch
			    || !wcschr(L"/*", src[i + 1])) {
				break;
			}
			switch (src[++i]) {
			case L'/':
				inlncmt = true;
				break;
			case L'*':
				inblkcmt = true;
				break;
			default:
				break;
			}
			break;
		case L'*':
			if (!inblkcmt || i + 1 >= firstch || src[i + 1] != L'/')
				break;
			inblkcmt = false;
			break;
		default:
			break;
		}
	}

	stk_unsigned_destroy(&pstk_open);
	stk_unsigned_destroy(&pstk_close);
	
	if (!inblkcmt && !inlncmt) {
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
	
	bool inblkcmt = false, inlncmt = false;
	bool instr = false, inch = false;
	while (firstspc + off < ln
	       && (src[firstspc + off] != L'(' || instr || inch || inblkcmt || inlncmt)) {
		wchar_t wch = src[firstspc + off];
		
		if (wch == L'\\' && (instr || inch) && !inblkcmt && !inlncmt) {
			if (src[firstspc + ++off] == L'\n')
				instr = inch = false;
			continue;
		} else if (wch == L'"' && !inch && !inblkcmt && !inlncmt)
			instr = !instr;
		else if (wch == L'\'' && !instr && !inblkcmt && !inlncmt)
			inch = !inch;
		else if (wch == L'\n' && !inblkcmt)
			inlncmt = instr = inch = false;
		else if (!wcsncmp(&src[firstspc + off], L"//", 2)
		         && !inch
		         && !instr
		         && !inblkcmt) {
			++off;
			inlncmt = true;
		} else if (!wcsncmp(&src[firstspc + off], L"/*", 2)
		           && !inch
		           && !instr
		           && !inlncmt) {
			++off;
			inblkcmt = true;
		} else if (!wcsncmp(&src[firstspc + off], L"*/", 2)
		           && inblkcmt) {
			++off;
			inblkcmt = false;
		}
		
		++off;
	}
	
	unsigned nopen = 0;
	for (size_t i = firstspc + off; i < ln; ++i) {
		wchar_t wch = src[i];
		
		if (wch == L'\\' && (instr || inch) && !inblkcmt && !inlncmt) {
			if (src[++i] == L'\n')
				instr = inch = false;
			continue;
		} else if (wch == L'"' && !inch && !inblkcmt && !inlncmt)
			instr = !instr;
		else if (wch == L'\'' && !instr && !inblkcmt && !inlncmt)
			inch = !inch;
		else if (wch == L'\n' && !inblkcmt)
			inlncmt = instr = inch = false;
		else if (!wcsncmp(&src[i], L"//", 2)
		         && !inch
		         && !instr
		         && !inblkcmt) {
			++i;
			inlncmt = true;
		} else if (!wcsncmp(&src[i], L"/*", 2)
		           && !inch
		           && !instr
		           && !inlncmt) {
			++i;
			inblkcmt = true;
		} else if (!wcsncmp(&src[i], L"*/", 2) && inblkcmt) {
			++i;
			inblkcmt = false;
		}
		
		nopen += src[i] == L'(' && !instr && !inch && !inblkcmt && !inlncmt;
		nopen -= src[i] == L')' && !instr && !inch && !inblkcmt && !inlncmt && nopen > 0;
	}
	
	if (firstch < mf->buf->size
	    && src[firstch] != L'{'
	    && src[firstch] != L'}'
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
	bool inblkcmt = false, inlncmt = false;
	bool instr = false, inch = false;
	for (size_t i = 0; i < pos; ++i) {
		wchar_t wch = mf->buf->conts[i];
		
		if (wch == L'\\' && (instr || inch) && !inblkcmt && !inlncmt) {
			if (mf->buf->conts[++i] == L'\n')
				instr = inch = false;
			continue;
		} else if (wch == L'"' && !inch && !inblkcmt && !inlncmt)
			instr = !instr;
		else if (wch == L'\'' && !instr && !inblkcmt && !inlncmt)
			inch = !inch;
		else if (wch == L'\n' && !inblkcmt)
			inlncmt = instr = inch = false;
		else if (!wcsncmp(&mf->buf->conts[i], L"//", 2)
		         && !inch
		         && !instr
		         && !inblkcmt) {
			++i;
			inlncmt = true;
		} else if (!wcsncmp(&mf->buf->conts[i], L"/*", 2)
		           && !inch
		           && !instr
		           && !inlncmt) {
			++i;
			inblkcmt = true;
		} else if (!wcsncmp(&mf->buf->conts[i], L"*/", 2) && inblkcmt) {
			++i;
			inblkcmt = false;
		}

		nopen += !instr && !inch && !inblkcmt && !inlncmt && wch == open;
		nopen -= !instr && !inch && !inblkcmt && !inlncmt && wch == close;
	}

	return nopen;
}
