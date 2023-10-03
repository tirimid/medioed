#include "keybd.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>
#include <unistd.h>

#include "util.h"

#define ESSPECKEY_NAME_BACKSPC "BACKSPC"
#define ESSPECKEY_NAME_SPC "SPC"
#define ESSPECKEY_NAME_ENTER "ENTER"
#define IGNORE_KEY 0x12349876
#define MAXBINDSIZE 128

struct keybind {
	char *keyseq;
	void (*fn)(void);
};

static char const *esspeckey_to_termkey(char const **essk);
static char *eskeyseq_to_termkeyseq(char const *eskeyseq);

static struct arraylist binds;
static char cur_bind[MAXBINDSIZE];

void
keybd_init(void)
{
	binds = arraylist_create();
}

void
keybd_quit(void)
{
	cur_bind[0] = 0;
	arraylist_destroy(&binds);
}

void
keybd_bind(char const *keyseq, void (*fn)(void))
{
	for (size_t i = 0; i < binds.size; ++i) {
		struct keybind *ex = binds.data[i];
		char *tkeyseq = eskeyseq_to_termkeyseq(keyseq);

		if (!strcmp(ex->keyseq, tkeyseq)) {
			ex->fn = fn;
			free(tkeyseq);
			return;
		}

		free(tkeyseq);
	}

	struct keybind new = {
		.keyseq = eskeyseq_to_termkeyseq(keyseq),
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

	strcat(cur_bind, kname);
	strcat(cur_bind, " ");

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

static char const *
esspeckey_to_termkey(char const **essk)
{
	size_t essk_len = 0;
	char const *essk_begin = *essk + 1;
	for (++*essk; **essk != '>'; ++*essk)
		++essk_len;

	if (!strncmp(ESSPECKEY_NAME_BACKSPC, essk_begin, essk_len))
		return "^?";
	if (!strncmp(ESSPECKEY_NAME_SPC, essk_begin, essk_len))
		return " ";
	if (!strncmp(ESSPECKEY_NAME_ENTER, essk_begin, essk_len))
		return "\n";
	else
		return "";
}

// note that this will only work for a subset of keysequences that would be
// valid when represented by the terminal keynames.
// for example, this will not work for "C-a" vs "C-A", as the underlying "^A"
// terminal keychord is unable to differentiate between upper/lowercase.
// also, note that this has no error condition and does not perform any
// validation for the passed emacs-style keysequence.
static char *
eskeyseq_to_termkeyseq(char const *eskeyseq)
{
	struct string tks = string_create();

	for (char const *c = eskeyseq; *c; ++c) {
		switch (*c) {
		case ' ':
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
			continue;
		case '<':
			string_push_str(&tks, esspeckey_to_termkey(&c));
			break;
		case 'C':
			if (*(c + 1) != '-') {
				string_push_ch(&tks, *c);
				break;
			}

			c += 2;
			string_push_ch(&tks, '^');
			string_push_ch(&tks, toupper(*c));

			break;
		case 'M':
			if (*(c + 1) != '-') {
				string_push_ch(&tks, *c);
				break;
			}

			++c;
			string_push_str(&tks, "^[");

			break;
		default:
			string_push_ch(&tks, *c);
			break;
		}

		string_push_ch(&tks, ' ');
	}

	char *tks_str = string_to_str(&tks);
	string_destroy(&tks);

	return tks_str;
}
