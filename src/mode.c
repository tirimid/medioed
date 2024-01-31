#include "mode.h"

#include <stddef.h>
#include <string.h>

#include <unistd.h>

#include "conf.h"

static ssize_t cur_mode = -1;

struct mode const *
mode_get(void)
{
	return cur_mode == -1 ? NULL : &conf_lmtab[cur_mode];
}

void
mode_set(char const *name, struct frame *f)
{
	struct mode const *old_mode = mode_get();
	
	if (!name) {
		if (old_mode)
			old_mode->quit();
		cur_mode = -1;
		return;
	}

	for (size_t i = 0; i < conf_lmtab_size; ++i) {
		if (!strcmp(name, conf_lmtab[i].name)) {
			if (old_mode)
				old_mode->quit();

			cur_mode = i;
			conf_lmtab[i].init(f);

			return;
		}
	}
	
	if (old_mode)
		old_mode->quit();
	cur_mode = -1;
}

void
mode_update(void)
{
	if (cur_mode != -1)
		conf_lmtab[cur_mode].update();
}

void
mode_key_update(wint_t k)
{
	if (cur_mode != -1)
		conf_lmtab[cur_mode].keypress(k);
}
