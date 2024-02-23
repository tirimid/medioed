#include "mode/mutil.h"

#include <stdint.h>
#include <string.h>

#include <unistd.h>

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
static ssize_t get_nearest_reg_fwd(struct vec_mu_region const *regs, size_t pos);
static ssize_t get_nearest_reg_back(struct vec_mu_region const *regs, size_t pos);

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

VEC_DEF_IMPL(struct mu_region, mu_region)

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

void
mu_finish_indent(size_t ln, size_t first_ch, unsigned ntab, unsigned nspace)
{
	buf_erase(mf->buf, ln, first_ch);
	for (unsigned i = 0; i < ntab; ++i)
		buf_write_wch(mf->buf, ln + i, L'\t');
	for (unsigned i = 0; i < nspace; ++i)
		buf_write_wch(mf->buf, ln + ntab + i, L' ');
	
	if (mf->csr <= first_ch)
		mf->csr = ln;
	else {
		mf->csr -= first_ch - ln;
		mf->csr_want_col -= first_ch - ln;
	}
	frame_mv_csr_rel(mf, 0, ntab + nspace, false);
}

size_t
mu_get_ln(size_t pos, struct vec_mu_region const *skip)
{
	size_t ln = pos;
	ssize_t reg_ind = get_nearest_reg_back(skip, ln);
	
	while (ln > 0) {
		struct mu_region const *reg = reg_ind == -1 ? NULL : &skip->data[reg_ind];
		
		if (mf->buf->conts[ln - 1] == L'\n') {
			if (reg && (ln < reg->lb || ln > reg->ub) || !reg)
				break;
		}
		
		if (reg && ln == reg->lb)
			--reg_ind;
		
		--ln;
	}
	
	return ln;
}

size_t
mu_get_ln_end(size_t pos, struct vec_mu_region const *skip)
{
	size_t ln_end = pos;
	ssize_t reg_ind = get_nearest_reg_fwd(skip, ln_end);
	
	while (ln_end < mf->buf->size) {
		struct mu_region const *reg = reg_ind >= skip->size ? NULL : &skip->data[reg_ind];
		
		if (mf->buf->conts[ln_end] == L'\n') {
			if (reg
			    && (ln_end < reg->lb || ln_end > reg->ub)
			    || !reg) {
				break;
			}
		}
		
		if (reg && ln_end == reg->ub)
			++reg_ind;
		
		++ln_end;
	}
	
	return ln_end;
}

size_t
mu_get_prev_ln(size_t pos, struct vec_mu_region const *skip)
{
	size_t prev_ln = mu_get_ln(pos, skip);
	
	prev_ln -= prev_ln > 0;
	if (prev_ln)
		prev_ln = mu_get_ln(prev_ln, skip);
	
	return prev_ln;
}

size_t
mu_get_first(size_t pos, struct vec_mu_region const *skip,
             bool (*is_sig)(wchar_t))
{
	size_t first_ch = pos;
	ssize_t reg_ind = get_nearest_reg_fwd(skip, first_ch);
	
	while (first_ch < mf->buf->size) {
		struct mu_region const *reg = reg_ind >= skip->size ? NULL : &skip->data[reg_ind];
		
		if (mf->buf->conts[first_ch] == L'\n'
		    || is_sig(mf->buf->conts[first_ch])) {
			if (reg
			    && (first_ch < reg->lb || first_ch > reg->ub)
			    || !reg) {
				break;
			}
		}
		
		if (reg && first_ch == reg->ub)
			++reg_ind;
		
		++first_ch;
	}
	
	return first_ch;
}

size_t
mu_get_last(size_t pos, struct vec_mu_region const *skip,
            bool (*is_sig)(wchar_t))
{
	size_t last_ch = pos;
	ssize_t reg_ind = get_nearest_reg_back(skip, last_ch);
	
	while (last_ch > 0) {
		struct mu_region const *reg = reg_ind == -1 ? NULL : &skip->data[reg_ind];
		
		if (mf->buf->conts[last_ch - 1] == L'\n'
		    || is_sig(mf->buf->conts[last_ch])) {
			if (reg
			    && (last_ch < reg->lb || last_ch > reg->ub)
			    || !reg) {
				break;
			}
		}
		
		if (reg && last_ch == reg->lb)
			--reg_ind;
		
		--last_ch;
	}
	
	return last_ch;
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

static ssize_t
get_nearest_reg_fwd(struct vec_mu_region const *regs, size_t pos)
{
	if (!regs->size)
		return -1;
	
	ssize_t nearest = -1;
	size_t min_dst = SIZE_MAX;
	for (size_t i = regs->size; i > 0; --i) {
		if (pos >= regs->data[i - 1].lb && pos <= regs->data[i - 1].ub)
			return i - 1;
		
		if (regs->data[i - 1].lb > pos
		    && regs->data[i - 1].lb - pos < min_dst) {
			nearest = i - 1;
			min_dst = regs->data[i - 1].lb - pos;
		} else
			break;
	}
	
	return nearest;
}

static ssize_t
get_nearest_reg_back(struct vec_mu_region const *regs, size_t pos)
{
	if (!regs->size)
		return -1;
	
	ssize_t nearest = -1;
	size_t min_dst = SIZE_MAX;
	for (size_t i = 0; i < regs->size; ++i) {
		if (pos >= regs->data[i - 1].lb && pos <= regs->data[i - 1].ub)
			return i;
		
		if (regs->data[i].ub < pos
		    && pos - regs->data[i].ub < min_dst) {
			nearest = i;
			min_dst = pos - regs->data[i].ub;
		} else
			break;
	}
	
	return nearest;
}
