#include "mode/mode_c.h"

#include <wctype.h>

#include "buf.h"
#include "keybd.h"
#include "util.h"

static void bind_indent(void);
static void bind_newline(void);

static struct frame *mf;

static int c_bind_indent[] = {K_TAB, -1};
static int c_bind_newline[] = {K_RET, -1};

void
mode_c_init(struct frame *f)
{
	mf = f;
	
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
bind_indent(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;

	// figure out identation parameters.
	size_t ln = mf->csr;
	while (ln > 0 && mf->buf->conts[ln - 1] != L'\n')
		--ln;

	size_t firstch = ln;
	while (firstch < mf->buf->size
	       && mf->buf->conts[firstch] != L'\n'
	       && iswspace(mf->buf->conts[firstch])) {
		++firstch;
	}
	
	unsigned ntab = 0;
	if (mf->buf->conts[firstch] != L'#') {
		for (size_t i = 0; i < ln; ++i) {
			ntab += mf->buf->conts[i] == L'{';
			ntab -= (mf->buf->conts[i] == L'}') * (ntab > 0);
		}
	}
	for (size_t i = firstch; ntab > 0 && i < mf->buf->size && mf->buf->conts[i] == L'}'; ++i)
		--ntab;
	
	// do indentation.
	buf_erase(mf->buf, ln, firstch);
	for (unsigned i = 0; i < ntab; ++i)
		buf_writewch(mf->buf, ln, L'\t');

	// fix cursor.
	if (mf->csr <= firstch)
		mf->csr = ln + ntab;
	else {
		mf->csr -= firstch - ln;
		mf->csr += ntab;
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
