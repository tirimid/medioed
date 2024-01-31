#include "mode/mode_s.h"

#include "conf.h"
#include "editor.h"
#include "keybd.h"
#include "mode/mutil.h"
#include "prompt.h"
#include "util.h"

static void bind_indent(void);
static void bind_new_line(void);

static struct frame *mf;
static uint8_t old_buf_flags;

static int s_bind_indent[] = {K_TAB, -1};

void
mode_s_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_set_base();
	
	keybd_bind(conf_bind_new_line, bind_new_line);
	keybd_bind(s_bind_indent, bind_indent);
	
	keybd_organize();
	
	old_buf_flags = mf->buf->flags;
	mf->buf->flags &= ~BF_WRITABLE;
	prompt_show(L"assembly mode is not implemented yet. buffer will be read-only");
	editor_redraw();
}

void
mode_s_quit(void)
{
	mf->buf->flags = old_buf_flags;
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
	
	
	// TODO: implement assembly indentation.
}

static void
bind_new_line(void)
{
	bind_indent();

	buf_push_hist_brk(mf->buf);
	buf_write_wch(mf->buf, mf->csr, L'\n');
	frame_mv_csr_rel(mf, 0, !!(mf->buf->flags & BF_WRITABLE), true);
	
	bind_indent();
}
