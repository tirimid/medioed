#include "mode/mode_cs.h"

#include "keybd.h"
#include "mode/mutil.h"

static void bind_indent(void);

static struct frame *mf;

static int cs_bind_indent[] = {K_TAB, -1};

void
mode_cs_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_set_base();
	mu_set_pairing(PF_PAREN | PF_BRACKET | PF_BRACE | PF_ANGLE | PF_SQUOTE | PF_DQUOTE);
	mu_set_bind_new_line(bind_indent);
	
	keybd_bind(cs_bind_indent, bind_indent);
	
	keybd_organize();
}

void
mode_cs_quit(void)
{
}

void
mode_cs_update(void)
{
}

void
mode_cs_keypress(wint_t k)
{
}

static void
bind_indent(void)
{
	// TODO: implement.
}
