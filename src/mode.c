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
	
	if (!name) {
		if (oldm)
			oldm->quit();
		curmode = -1;
		return;
	}

	for (size_t i = 0; i < conf_lmtab_size; ++i) {
		if (!strcmp(name, conf_lmtab[i].name)) {
			if (oldm)
				oldm->quit();

			curmode = i;
			conf_lmtab[i].init(f);

			return;
		}
	}
	
	if (oldm)
		oldm->quit();
	curmode = -1;
}

void
mode_update(void)
{
	if (curmode != -1)
		conf_lmtab[curmode].update();
}

void
mode_keyupdate(wint_t k)
{
	if (curmode != -1)
		conf_lmtab[curmode].keypress(k);
}
