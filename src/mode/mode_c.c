#include "mode/mode_c.h"

#include <string.h>
#include <wctype.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"
#include "util.h"

static void bind_pair_paren(void);
static void bind_pair_bracket(void);
static void bind_pair_brace(void);
static void bind_delback_ch(void);
static void bind_indent(void);
static void bind_newline(void);

static struct frame *mf;

static int c_bind_pair_paren[] = {'(', -1};
static int c_bind_pair_bracket[] = {'[', -1};
static int c_bind_pair_brace[] = {'{', -1};
static int c_bind_indent[] = {K_TAB, -1};
static int c_bind_newline[] = {K_RET, -1};

void
mode_c_init(struct frame *f)
{
	mf = f;

	keybd_bind(c_bind_pair_paren, bind_pair_paren);
	keybd_bind(c_bind_pair_bracket, bind_pair_bracket);
	keybd_bind(c_bind_pair_brace, bind_pair_brace);
	keybd_bind(conf_bind_delback_ch, bind_delback_ch);
	keybd_bind(c_bind_indent, bind_indent);
	keybd_bind(c_bind_newline, bind_newline);
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
bind_pair_paren(void)
{
	buf_writewstr(mf->buf, mf->csr, L"()");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_bracket(void)
{
	buf_writewstr(mf->buf, mf->csr, L"[]");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_brace(void)
{
	buf_writewstr(mf->buf, mf->csr, L"{}");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_delback_ch(void)
{
	if (mf->csr > 0 && mf->buf->flags & BF_WRITABLE) {
		frame_relmvcsr(mf, 0, -1, true);
		
		size_t nch = 1;
		if (!wcsncmp(mf->buf->conts + mf->csr, L"()", 2)
		    || !wcsncmp(mf->buf->conts + mf->csr, L"[]", 2)
		    || !wcsncmp(mf->buf->conts + mf->csr, L"{}", 2)) {
			nch = 2;
		}
			
		buf_erase(mf->buf, mf->csr, mf->csr + nch);
		frame_compbndry(mf);
	}
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
	
	unsigned ntab = 0;
	if (src[firstch] != L'#') {
		for (size_t i = 0; i < ln; ++i) {
			ntab += src[i] == L'{';
			ntab -= (src[i] == L'}') * (ntab > 0);
		}
	}
	for (size_t i = firstch; ntab > 0 && i < mf->buf->size && src[i] == L'}'; ++i)
		--ntab;

	size_t prevlastch = ln - 2 * (ln > 0);
	while (prevlastch > prevln && iswspace(prevlastch))
		--prevlastch;

	size_t prevfirstch = prevln;
	while (prevfirstch < mf->buf->size && iswspace(src[prevfirstch]))
		++prevfirstch;
	
	if (src[prevfirstch] != L'#' && src[prevlastch] == L')') {
		int nopen = 0;
		for (size_t i = 0; i <= prevlastch; ++i) {
			nopen += src[i] == L'(';
			nopen -= src[i] == L')';
		}
		if (nopen == 0 && src[firstch] != L'{' && src[firstch] != L'}')
			++ntab;
	} else if (src[prevfirstch] == L'#' && src[prevlastch] == L'\\')
		++ntab;

	unsigned nspace = 0;
	if (prevln != ln) {
		unsigned prevntab = 0, off = 0;
		size_t firstspace = prevln;
		
		while (src[firstspace] == L'\t') {
			++firstspace;
			++prevntab;
		}
		while (src[firstspace + off] == L' ')
			++off;
		
		if (src[firstch] != L'{'
		    && src[firstch] != L'}'
		    && off != 0
		    && !iswspace(src[firstspace + off])
		    && ntab == prevntab) {
			nspace = off;
		} else if (off == 0 && ntab == prevntab) {
			while (firstspace + off < ln
			       && src[firstspace + off] != L'(') {
				++off;
			}
			
			int nopen = 0;
			for (size_t i = firstspace + off; i < ln; ++i) {
				nopen += src[i] == L'(';
				nopen -= src[i] == L')';
			}
			
			if (src[firstch] != L'{'
			    && src[firstch] != L'}'
			    && firstspace + off < ln
			    && nopen > 0) {
				nspace = off + 1;
			}
		}
	}
	
	// do indentation.
	buf_erase(mf->buf, ln, firstch);
	for (unsigned i = 0; i < ntab; ++i)
		buf_writewch(mf->buf, ln, L'\t');
	for (unsigned i = 0; i < nspace; ++i)
		buf_writewch(mf->buf, ln + ntab, L' ');

	// fix cursor.
	if (mf->csr <= firstch)
		mf->csr = ln + ntab + nspace;
	else {
		mf->csr -= firstch - ln;
		mf->csr += ntab + nspace;
	}
}

static void
bind_newline(void)
{
	bind_indent();

	buf_writewch(mf->buf, mf->csr, L'\n');
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
	
	bind_indent();
}
