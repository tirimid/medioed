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
#define INDENT_SIZE 2

static int open_diff(size_t ln, size_t ln_end);
static void bind_indent(void);
static void bind_new_line(void);
static void bind_toggle_auto_indent(void);
static void bind_mk_tag(void);

static struct frame *mf;
static bool auto_indent;

static int html_bind_indent[] = {K_TAB, -1};
static int html_bind_toggle_auto_indent[] = {K_CTL('w'), 'i', -1};
static int html_bind_mk_tag[] = {K_CTL('w'), K_CTL('t'), -1};

void
mode_html_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_set_base();
	mu_set_pairing(PF_ANGLE | PF_DQUOTE);
	
	keybd_bind(conf_bind_new_line, bind_new_line);
	keybd_bind(html_bind_indent, bind_indent);
	keybd_bind(html_bind_toggle_auto_indent, bind_toggle_auto_indent);
	keybd_bind(html_bind_mk_tag, bind_mk_tag);
	
	keybd_organize();
	
	auto_indent = true;
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
open_diff(size_t ln, size_t ln_end)
{
	wchar_t const *src = mf->buf->conts;
	int diff = 0;
	
	for (size_t i = ln; i < ln_end; ++i) {
		if (i + 1 < ln_end && src[i] == L'<') {
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
		else if (i + 1 < ln_end && !wcsncmp(&src[i], L"/>", 2))
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
	
	size_t ln = mf->csr;
	while (ln > 0 && src[ln - 1] != L'\n')
		--ln;
	
	size_t first_ch = ln;
	while (first_ch < mf->buf->size
	       && src[first_ch] != L'\n'
	       && iswspace(src[first_ch])) {
		++first_ch;
	}
	
	unsigned nopen = 0;
	for (size_t i = 0; i < ln; ++i) {
		size_t ln_end = i;
		while (ln_end < mf->buf->size && src[ln_end] != L'\n')
			++ln_end;
		
		int diff = open_diff(i, ln_end);
		if (nopen && diff < 0)
			--nopen;
		else
			nopen += diff;
		
		i = ln_end;
	}
	
	size_t ln_end = ln;
	while (ln_end < mf->buf->size && src[ln_end] != L'\n')
		++ln_end;
	
	int diff = open_diff(ln, ln_end);
	if (nopen && diff < 0)
		--nopen;
	
	mu_finish_indent(ln, first_ch, 0, INDENT_SIZE * nopen);
}

static void
bind_new_line(void)
{
	if (auto_indent)
		bind_indent();
	
	buf_push_hist_brk(mf->buf);
	
	if (auto_indent
	    && mf->csr > 0
	    && !wcsncmp(&mf->buf->conts[mf->csr - 1], L"><", 2)) {
		buf_write_wstr(mf->buf, mf->csr, L"\n\n");
		mf->csr += 2;
		bind_indent();
		frame_mv_csr_rel(mf, -1, 0, false);
	} else {
		buf_write_wch(mf->buf, mf->csr, L'\n');	
		frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
	}
	
	if (auto_indent)
		bind_indent();
}

static void
bind_toggle_auto_indent(void)
{
	auto_indent = !auto_indent;
}

static void
bind_mk_tag(void)
{
	if (!(mf->buf->flags & BF_WRITABLE))
		return;
	
ask_again:;
	wchar_t *tag = prompt_ask(L"make tag: ", NULL);
	editor_redraw();
	if (!tag)
		return;
	
	if (!*tag) {
		free(tag);
		prompt_show(L"cannot insert blank tag!");
		editor_redraw();
		goto ask_again;
	}
	
	size_t tag_len = wcslen(tag);
	
	for (size_t i = 0; i < tag_len; ++i) {
		if (wcschr(L"<>", tag[i])) {
			free(tag);
			prompt_show(L"tag contains invalid characters!");
			editor_redraw();
			goto ask_again;
		}
	}
	
	size_t cls_tag_len = 0;
	while (iswalnum(tag[cls_tag_len]))
		++cls_tag_len;
	
	if (!cls_tag_len) {
		free(tag);
		prompt_show(L"cannot insert unclosable tag!");
		editor_redraw();
		goto ask_again;
	}
	
	wchar_t *cls_tag = malloc(sizeof(wchar_t) * (cls_tag_len + 1));
	memcpy(cls_tag, tag, sizeof(wchar_t) * cls_tag_len);
	cls_tag[cls_tag_len] = 0;
	
	size_t open_off = mf->csr;
	buf_write_wch(mf->buf, open_off, L'<');
	++open_off;
	buf_write_wstr(mf->buf, open_off, tag);
	open_off += tag_len;
	buf_write_wch(mf->buf, open_off, L'>');
	++open_off;
	
	size_t cls_off = open_off;
	buf_write_wstr(mf->buf, cls_off, L"</");
	cls_off += 2;
	buf_write_wstr(mf->buf, cls_off, cls_tag);
	cls_off += cls_tag_len;
	buf_write_wch(mf->buf, cls_off, L'>');
	++cls_off;
	
	frame_mv_csr_rel(mf, 0, open_off - mf->csr, false);
}
