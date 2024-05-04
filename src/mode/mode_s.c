#include "mode/mode_s.h"

#include "mode/mutil.h"

static struct frame *mf;

void
mode_s_init(struct frame *f)
{
	mf = f;
	mu_init(f);
	
	mu_set_base();
	mu_set_pairing(PF_BRACKET | PF_SQUOTE | PF_DQUOTE);
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
