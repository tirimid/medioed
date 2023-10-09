#include "mode.h"

#include <stddef.h>
#include <string.h>

#include <unistd.h>

#include "conf.h"

static ssize_t curmode = -1;

struct mode const *
mode_get(void)
{
	return curmode == -1 ? NULL : &conf_lmtab[curmode];
}

void
mode_set(char const *name, struct frame *f)
{
	struct mode const *oldm = mode_get();

	for (size_t i = 0; i < conf_lmtab_size; ++i) {
		if (!strcmp(name, conf_lmtab[i].name)) {
			if (oldm)
				oldm->quit(f);

			curmode = i;
			conf_lmtab[i].init(f);

			return;
		}
	}

	curmode = -1;
}

void
mode_keyupdate(struct frame *f, wint_t k)
{
	if (curmode != -1)
		conf_lmtab[curmode].keypress(f, k);
}
