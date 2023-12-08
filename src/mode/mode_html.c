#include "mode/mode_html.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "keybd.h"
#include "mode/mutil.h"

// HTML code typically gets significantly more indented than other computer
// languages, so instead of indenting with tabs (which would be ideal), a
// configurable number of spaces is used to allow a lesser indent than normal.
// also, this slightly better mirrors emacs behavior.
#define INDENTSIZE 2

static void bind_indent(void);
static void bind_newline(void);
static void bind_toggleautoindent(void);

static struct frame *mf;
static bool autoindent;

static int html_bind_indent[] = {K_TAB, -1};
static int html_bind_toggleautoindent[] = {K_CTL('w'), 'h', 'i', -1};

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
	// TODO: figure shit out.
	
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
}

static void
bind_toggleautoindent(void)
{
	autoindent = !autoindent;
}
