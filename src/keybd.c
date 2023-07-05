#include "keybd.h"

#define _POSIX_C_SOURCE 200809
#include <string.h>
#undef _POSIX_C_SOURCE

#include <stdlib.h>

#include <unistd.h>
#include <ncurses.h>
#include <tmcul/ds/arraylist.h>

struct keybind {
	char *keyseq;
	void (*fn)(void);
};

static struct arraylist binds;
static char cur_bind[128];

void
keybd_init(void)
{
	binds = arraylist_create();
}

void
keybd_quit(void)
{
	arraylist_destroy(&binds);
}

void
keybd_bind(char const *keyseq, void (*fn)(void))
{
	for (size_t i = 0; i < binds.size; ++i) {
		struct keybind *ex = binds.data[i];
		
		if (!strcmp(ex->keyseq, keyseq)) {
			ex->fn = fn;
			return;
		}
	}

	struct keybind new = {
		.keyseq = strdup(keyseq),
		.fn = fn,
	};

	arraylist_add(&binds, &new, sizeof(new));
}

void
keybd_unbind(char const *keyseq)
{
	for (size_t i = 0; i < binds.size; ++i) {
		struct keybind *bind = binds.data[i];
		
		if (!strcmp(bind->keyseq, keyseq)) {
			free(bind->keyseq);
			arraylist_rm(&binds, i);
			return;
		}
	}
}

int
keybd_await_input(void)
{
	int k = getch();
	char const *kname = keyname(k);

	sprintf(cur_bind, "%s%s ", cur_bind, kname);
	
	for (size_t i = 0; i < binds.size; ++i) {
		struct keybind const *bind = binds.data[i];
		
		if (!strcmp(bind->keyseq, cur_bind)) {
			bind->fn();
			cur_bind[0] = 0;
			return KEYBD_IGNORE_BIND;
		}

		if (!strncmp(bind->keyseq, cur_bind, strlen(cur_bind)))
			return KEYBD_IGNORE_BIND;
	}

	cur_bind[0] = 0;
	return k;
}
