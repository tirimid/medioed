#include "mode/mode_rs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "editor.h"
#include "keybd.h"
#include "mode/mutil.h"
#include "prompt.h"
#include "util.h"

static void bind_indent(void);
static void bind_newline(void);

static struct frame *mf;
static uint8_t oldbufflags;

static int rs_bind_indent[] = {K_TAB, -1};

void
mode_rs_init(struct frame *f)
{
	mf = f;
	mu_init(f);

	mu_setbase();
	mu_setpairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_ANGLE | PF_SQUOTE | PF_DQUOTE);

	keybd_bind(conf_bind_newline, bind_newline);
	keybd_bind(rs_bind_indent, bind_indent);

	keybd_organize();
	
	oldbufflags = mf->buf->flags;
	mf->buf->flags &= ~BF_WRITABLE;
	prompt_show(L"rust mode is not implemented yet. buffer will be read-only");
	editor_redraw();
}

void
mode_rs_quit(void)
{
	mf->buf->flags = oldbufflags;
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
	
	
	// TODO: implement rust indentation.
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
