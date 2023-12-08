#include "mode/mode_html.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "buf.h"
#include "conf.h"
#include "editor.h"
#include "keybd.h"
#include "mode/mutil.h"
#include "prompt.h"

// HTML code typically gets significantly more indented than other computer
// languages, so instead of indenting with tabs (which would be ideal), a
// configurable number of spaces is used to allow a lesser indent than normal.
// also, this slightly better mirrors emacs behavior.
#define INDENTSIZE 2

static int opendiff(size_t ln, size_t lnend);
static void bind_indent(void);
static void bind_newline(void);
static void bind_toggleautoindent(void);
static void bind_mktag(void);

static struct frame *mf;
static bool autoindent;

static int html_bind_indent[] = {K_TAB, -1};
static int html_bind_toggleautoindent[] = {K_CTL('w'), 'i', -1};
static int html_bind_mktag[] = {K_CTL('w'), K_CTL('t'), -1};

void
mode_html_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_setbase();
	mu_setpairing(PF_ANGLE | PF_DQUOTE);
	
	keybd_bind(conf_bind_newline, bind_newline);
	keybd_bind(html_bind_indent, bind_indent);
	keybd_bind(html_bind_toggleautoindent, bind_toggleautoindent);
	keybd_bind(html_bind_mktag, bind_mktag);
	
	keybd_organize();
	
	autoindent = true;
}

void
mode_html_quit(void)
{
}

void
mode_html_update(void)
{
}

void
mode_html_keypress(wint_t k)
{
}

static int
opendiff(size_t ln, size_t lnend)
{
	wchar_t const *src = mf->buf->conts;
	int diff = 0;
	
	for (size_t i = ln; i < lnend; ++i) {
		if (i + 1 < lnend && src[i] == L'<') {
			switch (src[i + 1]) {
			case L'!':
			case L'>':
				break;
			case L'/':
				--diff;
				break;
			default:
				++diff;
				break;
			}
		} else if (src[i] == L'<')
			++diff;
		else if (i + 1 < lnend && !wcsncmp(&src[i], L"/>", 2))
			--diff;
	}
	
	return diff ? SIGN(diff) : 0;
}

static void
bind_indent(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
	wchar_t const *src = mf->buf->conts;
	
	//  figure out indentation parameters.
	size_t ln = mf->csr;
	while (ln > 0 && src[ln - 1] != L'\n')
		--ln;
	
	size_t firstch = ln;
	while (firstch < mf->buf->size
	       && src[firstch] != L'\n'
	       && iswspace(src[firstch])) {
		++firstch;
	}
	
	unsigned nopen = 0;
	for (size_t i = 0; i < ln; ++i) {
		size_t lnend = i;
		while (lnend < mf->buf->size && src[lnend] != L'\n')
			++lnend;
		
		int diff = opendiff(i, lnend);
		if (nopen && diff < 0)
			--nopen;
		else
			nopen += diff;
		
		i = lnend;
	}
	
	size_t lnend = ln;
	while (lnend < mf->buf->size && src[lnend] != L'\n')
		++lnend;
	
	int diff = opendiff(ln, lnend);
	if (nopen && diff < 0)
		--nopen;
	
	// do indentation.
	buf_erase(mf->buf, ln, firstch);
	for (unsigned i = 0; i < INDENTSIZE * nopen; ++i)
		buf_writewch(mf->buf, ln + i, L' ');
	
	// fix cursor.
	if (mf->csr <= firstch)
		mf->csr = ln;
	else
		mf->csr -= firstch - ln;
	frame_relmvcsr(mf, 0, INDENTSIZE * nopen, false);
}

static void
bind_newline(void)
{
	if (autoindent)
		bind_indent();
	
	buf_pushhistbrk(mf->buf);
	
	if (autoindent
	    && mf->csr > 0
	    && !wcsncmp(&mf->buf->conts[mf->csr - 1], L"><", 2)) {
		buf_writewstr(mf->buf, mf->csr, L"\n\n");
		mf->csr += 2;
		bind_indent();
		frame_relmvcsr(mf, -1, 0, false);
	} else {
		buf_writewch(mf->buf, mf->csr, L'\n');	
		frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
	}
	
	if (autoindent)
		bind_indent();
}

static void
bind_toggleautoindent(void)
{
	autoindent = !autoindent;
}

static void
bind_mktag(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
askagain:;
	wchar_t *tag = prompt_ask(L"make tag: ", NULL, NULL);
	editor_redraw();
	if (!tag)
		return;
	
	if (!*tag) {
		free(tag);
		prompt_show(L"cannot insert blank tag!");
		editor_redraw();
		goto askagain;
	}
	
	size_t taglen = wcslen(tag);
	
	for (size_t i = 0; i < taglen; ++i) {
		if (wcschr(L"<>", tag[i])) {
			free(tag);
			prompt_show(L"tag contains invalid characters!");
			editor_redraw();
			goto askagain;
		}
	}
	
	size_t clstaglen = 0;
	while (iswalnum(tag[clstaglen]))
		++clstaglen;
	
	if (!clstaglen) {
		free(tag);
		prompt_show(L"cannot insert unclosable tag!");
		editor_redraw();
		goto askagain;
	}
	
	wchar_t *clstag = malloc(sizeof(wchar_t) * (clstaglen + 1));
	memcpy(clstag, tag, sizeof(wchar_t) * clstaglen);
	clstag[clstaglen] = 0;
	
	size_t openoff = mf->csr;
	buf_writewch(mf->buf, openoff, L'<');
	++openoff;
	buf_writewstr(mf->buf, openoff, tag);
	openoff += taglen;
	buf_writewch(mf->buf, openoff, L'>');
	++openoff;
	
	size_t clsoff = openoff;
	buf_writewstr(mf->buf, clsoff, L"</");
	clsoff += 2;
	buf_writewstr(mf->buf, clsoff, clstag);
	clsoff += clstaglen;
	buf_writewch(mf->buf, clsoff, L'>');
	++clsoff;
	
	frame_relmvcsr(mf, 0, openoff - mf->csr, false);
}
