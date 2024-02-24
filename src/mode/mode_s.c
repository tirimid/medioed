#include "mode/mode_s.h"

#include <wctype.h>

#include "buf.h"
#include "conf.h"
#include "keybd.h"
#include "mode/mutil.h"
#include "util.h"

static void bind_indent(void);

static struct frame *mf;

static int s_bind_indent[] = {K_TAB, -1};

void
mode_s_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_set_base();
	mu_set_pairing(PF_BRACKET | PF_SQUOTE | PF_DQUOTE);
	mu_set_bind_new_line(bind_indent);
	
	keybd_bind(s_bind_indent, bind_indent);
	
	keybd_organize();
}

void
mode_s_quit(void)
{
}

void
mode_s_update(void)
{
}

void
mode_s_keypress(wint_t k)
{
}

static void
bind_indent(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
	wchar_t const *src = mf->buf->conts;
	
	// since assembly is such a simple language with such few indentational
	// requirements, and GNU assembler is such a simple dialect of assembly,
	// the indentation determination can be extremely simple.
	
	size_t ln = mf->csr;
	while (ln > 0 && src[ln - 1] != L'\n')
		--ln;

	size_t first_ch = ln;
	while (first_ch < mf->buf->size
	       && src[first_ch] != L'\n'
	       && iswspace(src[first_ch])) {
		++first_ch;
	}
	
	size_t ln_end = mf->csr;
	while (ln_end < mf->buf->size && src[ln_end] != L'\n')
		++ln_end;
	
	size_t last_ch = ln_end - (ln_end > 0);
	while (last_ch > 0 && src[last_ch] != L'\n' && iswspace(src[last_ch]))
		--last_ch;
	
	unsigned ntab = 1;
	if (src[last_ch] == L':' || ln_end == mf->buf->size)
		ntab = 0;
	
	mu_finish_indent(ln, first_ch, ntab, 0);
}
