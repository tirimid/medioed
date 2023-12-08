#include "mode/mutil.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"

static void bind_popen_pn(void);
static void bind_popen_bk(void);
static void bind_popen_bc(void);
static void bind_popen_ag(void);
static void bind_pclose_pn(void);
static void bind_pclose_bk(void);
static void bind_pclose_bc(void);
static void bind_pclose_ag(void);
static void bind_pair_sq(void);
static void bind_pair_dq(void);
static void bind_delback_ch(void);

static struct frame *mf;
static bool opt_base = false, opt_pairing = false;
static unsigned long pairflags = 0;

static int mu_bind_popen_pn[] = {'(', -1};
static int mu_bind_popen_bk[] = {'[', -1};
static int mu_bind_popen_bc[] = {'{', -1};
static int mu_bind_popen_ag[] = {'<', -1};
static int mu_bind_pclose_pn[] = {')', -1};
static int mu_bind_pclose_bk[] = {']', -1};
static int mu_bind_pclose_bc[] = {'}', -1};
static int mu_bind_pclose_ag[] = {'>', -1};
static int mu_bind_pair_sq[] = {'\'', -1};
static int mu_bind_pair_dq[] = {'"', -1};

void
mu_init(struct frame *f)
{
	mf = f;
	opt_base = false;
	opt_pairing = false;
}

void
mu_setbase(void)
{
	if (opt_base)
		return;
	
	opt_base = true;
	
	keybd_bind(conf_bind_delback_ch, bind_delback_ch);
}

void
mu_setpairing(unsigned long flags)
{
	if (opt_pairing)
		return;
	
	opt_pairing = true;
	pairflags = flags;
	
	if (flags & PF_PAREN) {
		keybd_bind(mu_bind_popen_pn, bind_popen_pn);
		keybd_bind(mu_bind_pclose_pn, bind_pclose_pn);
	}
	
	if (flags & PF_BRACKET) {
		keybd_bind(mu_bind_popen_bk, bind_popen_bk);
		keybd_bind(mu_bind_pclose_bk, bind_pclose_bk);
	}
	
	if (flags & PF_BRACE) {
		keybd_bind(mu_bind_popen_bc, bind_popen_bc);
		keybd_bind(mu_bind_pclose_bc, bind_pclose_bc);
	}
	
	if (flags & PF_ANGLE) {
		keybd_bind(mu_bind_popen_ag, bind_popen_ag);
		keybd_bind(mu_bind_pclose_ag, bind_pclose_ag);
	}
	
	if (flags & PF_SQUOTE)
		keybd_bind(mu_bind_pair_sq, bind_pair_sq);
	
	if (flags & PF_DQUOTE)
		keybd_bind(mu_bind_pair_dq, bind_pair_dq);
	
	keybd_organize();
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
bind_popen_ag(void)
{
	buf_writewstr(mf->buf, mf->csr, L"<>");
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
bind_pclose_ag(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L'>')
		buf_writewch(mf->buf, mf->csr, L'>');
	frame_relmvcsr(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
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
bind_delback_ch(void)
{
	if (mf->csr > 0 && mf->buf->flags & BF_WRITABLE) {
		frame_relmvcsr(mf, 0, -1, true);
		
		size_t nch = 1;
		
		if (opt_pairing
		    && mf->buf->size > 1
		    && mf->csr < mf->buf->size - 1) {
			if (!wcsncmp(mf->buf->conts + mf->csr, L"()", 2) && (pairflags & PF_PAREN)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"[]", 2) && (pairflags & PF_BRACKET)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"{}", 2) && (pairflags & PF_BRACE)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"<>", 2) && (pairflags & PF_ANGLE)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"\"\"", 2) && (pairflags & PF_DQUOTE)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"''", 2) && (pairflags && PF_SQUOTE)) {
				nch = 2;
			}
		}
			
		buf_erase(mf->buf, mf->csr, mf->csr + nch);
		frame_compbndry(mf);
	}
}
