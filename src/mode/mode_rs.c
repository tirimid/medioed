#include "mode/mode_rs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "keybd.h"
#include "mode/mutil.h"
#include "util.h"

static void bind_indent(void);
static void bind_newline(void);
static long nopenat(size_t pos, wchar_t const *open, wchar_t const *close);

static struct frame *mf;

static int rs_bind_indent[] = {K_TAB, -1};

void
mode_rs_init(struct frame *f)
{
	mf = f;
	mu_init(f);

	mu_setbase();
	mu_setpairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_ANGLE | PF_DQUOTE);

	keybd_bind(conf_bind_newline, bind_newline);
	keybd_bind(rs_bind_indent, bind_indent);

	keybd_organize();
}

void
mode_rs_quit(void)
{
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
	
	wchar_t const *src = mf->buf->conts;
	
	// figure out indentation parameters.
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
	
	long ntab = nopenat(firstch, L"([{", L")]}");
	
	if (prevln != ln) {
		size_t prevlastch = ln - 1;
		while (prevlastch > prevln && iswspace(src[prevlastch]))
			--prevlastch;
		
		if (!iswspace(src[prevlastch])
		    && !iswspace(src[firstch])
		    && !wcschr(L"([{};,", src[prevlastch])
		    && !wcschr(L")]}", src[firstch])) {
			++ntab;
		}
	}
	
	for (size_t i = firstch; ntab > 0 && i < mf->buf->size && wcschr(L")]}", src[i]); ++i)
		--ntab;
	
	ntab = MAX(0, ntab);
	
	// do indentation.
	buf_erase(mf->buf, ln, firstch);
	for (unsigned i = 0; i < ntab; ++i)
		buf_writewch(mf->buf, ln + i, L'\t');
	
	// fix cursor.
	if (mf->csr <= firstch)
		mf->csr = ln;
	else {
		mf->csr -= firstch - ln;
		mf->csr_wantcol -= firstch - ln;
	}
	frame_relmvcsr(mf, 0, ntab, false);
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

static long
nopenat(size_t pos, wchar_t const *open, wchar_t const *close)
{
	long nopen = 0;
	
	wchar_t const *src = mf->buf->conts;
	bool inch = false, instr = false, inrstr = false;
	wchar_t *cmp = malloc(1);
	unsigned cmplen = 0;
	
	for (size_t i = 0; i < pos; ++i) {
		if (!inrstr
		    && !instr
		    && !inch
		    && i + 1 < pos
		    && src[i] == L'r'
		    && (src[i + 1] == L'#' || src[i + 1] == L'"')) {
			size_t j = i + 1;
			while (j < pos && src[j] == L'#')
				++j;
			
			if (j >= pos || src[j] != L'"')
				continue;
			
			j += j < pos;
			cmplen = j - i - 1;
			cmp = realloc(cmp, sizeof(wchar_t) * cmplen);
			cmp[0] = L'"';
			
			for (unsigned i = 1; i < cmplen; ++i)
				cmp[i] = L'#';
			
			inrstr = true;
		} else if (!inrstr && !inch && src[i] == L'"')
			instr = !instr;
		else if (!inrstr && !instr && src[i] == L'\'')
			inch = !inch;
		else if (inrstr
		         && i + cmplen < pos
		         && !wcsncmp(&src[i], cmp, cmplen)) {
			inrstr = false;
			i += cmplen;
		} else if ((instr || inch) && src[i] == L'\\') {
			++i;
			continue;
		}
		
		nopen += !inrstr && !instr && !inch && wcschr(open, src[i]);
		nopen -= !inrstr && !instr && !inch && wcschr(close, src[i]);
	}
	
	free(cmp);
	
	return nopen;
}
