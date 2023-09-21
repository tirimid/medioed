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
	struct mode const *oldm = mode_get();
	
	for (size_t i = 0; i < conf_lmtab_size; ++i) {
		struct mode const *m = &conf_lmtab[i];
		
		if (!strcmp(name, m->name)) {
			if (oldm)
				oldm->quit(f);

			cur_mode = i;
			m->init(f);
			
			return;
		}
	}

	cur_mode = -1;
}

void
mode_keyupdate(struct frame *f, int k)
{
	if (cur_mode == -1)
		return;

	struct mode *m = &conf_lmtab[cur_mode];
	m->keypress(f, k);
}
