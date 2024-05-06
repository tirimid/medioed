#include "mode/mode_clike.h"

#include <stdbool.h>
#include <wchar.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"
#include "mode/mutil.h"

static void bind_new_line(void);
static void bind_tab_align(void);
static void expand_pair(unsigned ntab);

static struct frame *mf;

static int clike_bind_tab_align[] = {K_CTL('w'), K_CTL('a'), -1};

void
mode_clike_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_set_base();
	mu_set_pairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_SQUOTE | PF_DQUOTE);
	
	keybd_bind(conf_bind_new_line, bind_new_line);
	keybd_bind(clike_bind_tab_align, bind_tab_align);
	
	keybd_organize();
}

void
mode_clike_quit(void)
{
}

void
mode_clike_update(void)
{
}

void
mode_clike_keypress(wint_t k)
{
}

static void
bind_new_line(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
	size_t ln = mf->csr;
	while (ln > 0 && buf_get_wch(mf->buf, ln - 1) != L'\n')
		--ln;
	
	unsigned ntab = 0;
	while (ln + ntab < mf->buf->size
	       && buf_get_wch(mf->buf, ln + ntab) == L'\t')
	{
		++ntab;
	}
	
	unsigned nspace = 0;
	while (ln + ntab + nspace < mf->buf->size
	       && buf_get_wch(mf->buf, ln + ntab + nspace) == L' ')
	{
		++nspace;
	}
	
	if (mf->csr > 0)
	{
		wchar_t cmp_buf[3];
		buf_get_wstr(mf->buf, cmp_buf, mf->csr - 1, 3);
		
		if (!wcscmp(L"()", cmp_buf)
		    || !wcscmp(L"[]", cmp_buf)
		    || !wcscmp(L"{}", cmp_buf))
		{
			expand_pair(ntab);
			return;
		}
	}
	
	buf_write_wch(mf->buf, mf->csr, L'\n');
	for (unsigned i = 0; i < ntab; ++i)
		buf_write_wch(mf->buf, mf->csr + 1 + i, L'\t');
	for (unsigned i = 0; i < nspace; ++i)
		buf_write_wch(mf->buf, mf->csr + 1 + ntab + i, L' ');
	
	++mf->csr;
	mf->csr_want_col = ntab + nspace;
	frame_mv_csr_rel(mf, 0, ntab + nspace, false);
}

static void
bind_tab_align(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
	size_t ln = mf->csr;
	while (ln > 0 && buf_get_wch(mf->buf, ln - 1) != L'\n')
		--ln;
	
	unsigned ntab = 0;
	while (ln + ntab < mf->buf->size
	       && buf_get_wch(mf->buf, ln + ntab) == L'\t')
	{
		++ntab;
	}
	
	unsigned nspace = 0;
	while (ln + ntab + nspace < mf->buf->size
	       && buf_get_wch(mf->buf, ln + ntab + nspace) == L' ')
	{
		++nspace;
	}
	
	buf_erase(mf->buf, ln + ntab, ln + ntab + nspace);
	if (mf->csr >= ln + ntab + nspace)
	{
		mf->csr -= nspace;
		mf->csr_want_col -= nspace;
	}
	else if (mf->csr > ln + ntab)
	{
		mf->csr = ln + ntab;
		mf->csr_want_col = ln + ntab;
	}
}

static void
expand_pair(unsigned ntab)
{
	size_t write_pos = mf->csr;
	
	buf_write_wch(mf->buf, write_pos++, L'\n');
	for (unsigned i = 0; i < ntab + 1; ++i)
		buf_write_wch(mf->buf, write_pos++, L'\t');
		
	buf_write_wch(mf->buf, write_pos++, L'\n');
	for (unsigned i = 0; i < ntab; ++i)
		buf_write_wch(mf->buf, write_pos++, L'\t');
	
	mf->csr += 2;
	frame_mv_csr_rel(mf, 0, ntab + 1, false);
}
