#include "mode/mutil.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"

static void bind_pair_open_pn(void);
static void bind_pair_open_bk(void);
static void bind_pair_open_bc(void);
static void bind_pair_open_ag(void);
static void bind_pair_close_pn(void);
static void bind_pair_close_bk(void);
static void bind_pair_close_bc(void);
static void bind_pair_close_ag(void);
static void bind_pair_sq(void);
static void bind_pair_dq(void);
static void bind_del_back_ch(void);

static struct frame *mf;
static bool opt_base = false, opt_pairing = false;
static unsigned long pair_flags = 0;

static int mu_bind_pair_open_pn[] = {'(', -1};
static int mu_bind_pair_open_bk[] = {'[', -1};
static int mu_bind_pair_open_bc[] = {'{', -1};
static int mu_bind_pair_open_ag[] = {'<', -1};
static int mu_bind_pair_close_pn[] = {')', -1};
static int mu_bind_pair_close_bk[] = {']', -1};
static int mu_bind_pair_close_bc[] = {'}', -1};
static int mu_bind_pair_close_ag[] = {'>', -1};
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
mu_set_base(void)
{
	if (opt_base)
		return;
	
	opt_base = true;
	
	keybd_bind(conf_bind_del_back_ch, bind_del_back_ch);
}

void
mu_set_pairing(unsigned long flags)
{
	if (opt_pairing)
		return;
	
	opt_pairing = true;
	pair_flags = flags;
	
	if (flags & PF_PAREN) {
		keybd_bind(mu_bind_pair_open_pn, bind_pair_open_pn);
		keybd_bind(mu_bind_pair_close_pn, bind_pair_close_pn);
	}
	
	if (flags & PF_BRACKET) {
		keybd_bind(mu_bind_pair_open_bk, bind_pair_open_bk);
		keybd_bind(mu_bind_pair_close_bk, bind_pair_close_bk);
	}
	
	if (flags & PF_BRACE) {
		keybd_bind(mu_bind_pair_open_bc, bind_pair_open_bc);
		keybd_bind(mu_bind_pair_close_bc, bind_pair_close_bc);
	}
	
	if (flags & PF_ANGLE) {
		keybd_bind(mu_bind_pair_open_ag, bind_pair_open_ag);
		keybd_bind(mu_bind_pair_close_ag, bind_pair_close_ag);
	}
	
	if (flags & PF_SQUOTE)
		keybd_bind(mu_bind_pair_sq, bind_pair_sq);
	
	if (flags & PF_DQUOTE)
		keybd_bind(mu_bind_pair_dq, bind_pair_dq);
	
	keybd_organize();
}

static void
bind_pair_open_pn(void)
{
	buf_write_wstr(mf->buf, mf->csr, L"()");
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_open_bk(void)
{
	buf_write_wstr(mf->buf, mf->csr, L"[]");
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_open_bc(void)
{
	buf_write_wstr(mf->buf, mf->csr, L"{}");
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_open_ag(void)
{
	buf_write_wstr(mf->buf, mf->csr, L"<>");
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_close_pn(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L')')
		buf_write_wch(mf->buf, mf->csr, L')');
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_close_bk(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L']')
		buf_write_wch(mf->buf, mf->csr, L']');
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_close_bc(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L'}')
		buf_write_wch(mf->buf, mf->csr, L'}');
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_close_ag(void)
{
	if (mf->csr >= mf->buf->size || mf->buf->conts[mf->csr] != L'>')
		buf_write_wch(mf->buf, mf->csr, L'>');
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_sq(void)
{
	buf_write_wstr(mf->buf, mf->csr, L"''");
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_pair_dq(void)
{
	buf_write_wstr(mf->buf, mf->csr, L"\"\"");
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
}

static void
bind_del_back_ch(void)
{
	if (mf->csr > 0 && mf->buf->flags & BF_WRITABLE) {
		frame_mv_csr_rel(mf, 0, -1, true);
		
		size_t nch = 1;
		
		if (opt_pairing
		    && mf->buf->size > 1
		    && mf->csr < mf->buf->size - 1) {
			if (!wcsncmp(mf->buf->conts + mf->csr, L"()", 2) && (pair_flags & PF_PAREN)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"[]", 2) && (pair_flags & PF_BRACKET)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"{}", 2) && (pair_flags & PF_BRACE)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"<>", 2) && (pair_flags & PF_ANGLE)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"\"\"", 2) && (pair_flags & PF_DQUOTE)
			    || !wcsncmp(mf->buf->conts + mf->csr, L"''", 2) && (pair_flags && PF_SQUOTE)) {
				nch = 2;
			}
		}
		
		buf_erase(mf->buf, mf->csr, mf->csr + nch);
		frame_comp_boundary(mf);
	}
}
