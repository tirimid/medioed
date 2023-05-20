#include "keybd.h"

#define _POSIX_C_SOURCE 200809
#include <string.h>
#undef _POSIX_C_SOURCE

#include <stdlib.h>

#include <unistd.h>
#include <ncurses.h>
#include <libtmcul/tmcul.h>

struct keybind {
	char *keyseq;
	void (*fn)(void);
};

static struct arraylist binds;
static ssize_t cur_bind = -1;
static size_t cur_bind_off = 0;

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
	size_t kname_len = strlen(kname);
	
	for (size_t i = 0; i < binds.size; ++i) {
		struct keybind const *bind = binds.data[i];
		if (strncmp(bind->keyseq + cur_bind_off, kname, kname_len))
			continue;
		
		cur_bind = i;
		cur_bind_off += kname_len;
		if (cur_bind_off == strlen(bind->keyseq)) {
			bind->fn();
			break;
		}
		
		goto bind_progress;
	}

	// reset bind information if no progress on a bind was made or the currently
	// in-use bind completed.
	cur_bind = -1;
	cur_bind_off = 0;
bind_progress:

	return cur_bind == -1 ? k : KEYBD_IGNORE;
}
