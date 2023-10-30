#include "mode/mode_c.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"
#include "util.h"

static void bind_pair_sq(void);
static void bind_pair_dq(void);
static void bind_popen_pn(void);
static void bind_popen_bk(void);
static void bind_popen_bc(void);
static void bind_pclose_pn(void);
static void bind_pclose_bk(void);
static void bind_pclose_bc(void);
static void bind_delback_ch(void);
static void bind_indent(void);
static void bind_newline(void);
static long nopenat(size_t pos, wchar_t open, wchar_t close, bool reqpos);

static struct frame *mf;

static int c_bind_pair_sq[] = {'\'', -1};
static int c_bind_pair_dq[] = {'"', -1};
static int c_bind_popen_pn[] = {'(', -1};
static int c_bind_popen_bk[] = {'[', -1};
static int c_bind_popen_bc[] = {'{', -1};
static int c_bind_pclose_pn[] = {')', -1};
static int c_bind_pclose_bk[] = {']', -1};
static int c_bind_pclose_bc[] = {'}', -1};
static int c_bind_indent[] = {K_TAB, -1};
static int c_bind_newline[] = {K_RET, -1};

void
mode_c_init(struct frame *f)
{
	mf = f;
	
	keybd_bind(c_bind_pair_sq, bind_pair_sq);
	keybd_bind(c_bind_pair_dq, bind_pair_dq);
	keybd_bind(c_bind_popen_pn, bind_popen_pn);
	keybd_bind(c_bind_popen_bk, bind_popen_bk);
	keybd_bind(c_bind_popen_bc, bind_popen_bc);
	keybd_bind(c_bind_pclose_pn, bind_pclose_pn);
	keybd_bind(c_bind_pclose_bk, bind_pclose_bk);
	keybd_bind(c_bind_pclose_bc, bind_pclose_bc);
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
bind_pair_sq(void)
{
	buf_writewstr(mf->buf, mf->csr, L"''");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_dq(void)
{
	buf_writewstr(mf->buf, mf->csr, L"\"\"");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_popen_pn(void)
{
	buf_writewstr(mf->buf, mf->csr, L"()");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_popen_bk(void)
{
	buf_writewstr(mf->buf, mf->csr, L"[]");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_popen_bc(void)
{
	buf_writewstr(mf->buf, mf->csr, L"{}");
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pclose_pn(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L')')
		buf_writewch(mf->buf, mf->csr, L')');
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pclose_bk(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L']')
		buf_writewch(mf->buf, mf->csr, L']');
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pclose_bc(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L'}')
		buf_writewch(mf->buf, mf->csr, L'}');
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_delback_ch(void)
{
	if (mf->csr > 0 && mf->buf->flags & BF_WRITABLE) {
		frame_relmvcsr(mf, 0, -1, true);
		
		size_t nch = 1;
		if (mf->buf->size > 1) {
			if (!wcsncmp(mf->buf->conts + mf->csr, L"()", 2)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"[]", 2)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"{}", 2)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"\"\"", 2)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"''", 2)) {
				nch = 2;
			}
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
	
	unsigned ntab = nopenat(firstch, L'{', L'}', true);
	for (size_t i = firstch; ntab > 0 && i < mf->buf->size && src[i] == L'}'; ++i)
		--ntab;

	unsigned nspace = 0;
	if (prevln != ln) {
		size_t prevlastch = ln - 2 * (ln > 0);
		while (prevlastch > prevln && iswspace(src[prevlastch]))
			--prevlastch;

		size_t prevfirstch = prevln;
		while (prevfirstch < mf->buf->size && iswspace(src[prevfirstch]))
			++prevfirstch;
		
		if (src[prevlastch] == L')' && src[prevfirstch] != L'#') {
			if (nopenat(prevlastch + 1, L'(', L')', false) == 0
			    && src[firstch] != L'{' && src[firstch] != L'}') {
				++ntab;
			}
		} else if (src[prevlastch] == L'\\')
			++ntab;
		
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
			// unfortunately, `nopenat()` cannot be used for this as
			// the functionality implemented here requires internal
			// knowledge that would be impossible to obtain via the
			// function call.
			bool instr = false, inch = false;
			while (firstspace + off < ln
			       && src[firstspace + off] != L'('
			       || instr
			       || inch) {
				wchar_t wch = src[firstspace + off];
				
				if (wch == L'\\' && (instr || inch)) {
					if (src[firstspace + ++off] == L'\n')
						instr = inch = false;
					continue;
				} else if (wch == L'"' && !inch)
					instr = !instr;
				else if (wch == L'\'' && !instr)
					inch = !inch;
				else if (wch == L'\n')
					instr = inch = false;
				
				++off;
			}
			
			long nopen = 0;
			for (size_t i = firstspace + off; i < ln; ++i) {
				wchar_t wch = src[firstspace + off];
				
				if (wch == L'\\' && (instr || inch)) {
					if (src[firstspace + ++off] == L'\n')
						instr = inch = false;
					continue;
				} else if (wch == L'"' && !inch)
					instr = !instr;
				else if (wch == L'\'' && !instr)
					inch = !inch;
				else if (wch == L'\n')
					instr = inch = false;
				
				nopen += src[i] == L'(' && !instr && !inch;
				nopen -= src[i] == L')' && !instr && !inch;
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
		mf->csr = ln;
	else
		mf->csr -= firstch - ln;
	frame_relmvcsr(mf, 0, ntab + nspace, false);
}

static void
bind_newline(void)
{
	bind_indent();

	buf_writewch(mf->buf, mf->csr, L'\n');
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
	
	bind_indent();
}

static long
nopenat(size_t pos, wchar_t open, wchar_t close, bool reqpos)
{
	long nopen = 0;
	bool instr = false, inch = false;
	for (size_t i = 0; i < pos; ++i) {
		wchar_t wch = mf->buf->conts[i];
		
		if (wch == L'\\' && (instr || inch)) {
			if (mf->buf->conts[++i] == L'\n')
				instr = inch = false;
			continue;
		} else if (wch == L'"' && !inch)
			instr = !instr;
		else if (wch == L'\'' && !instr)
			inch = !inch;
		else if (wch == L'\n')
			instr = inch = false;

		nopen += wch == open && !instr && !inch;
		nopen -= wch == close && !instr && !inch && (reqpos && nopen > 0 || !reqpos);
	}

	return nopen;
}
