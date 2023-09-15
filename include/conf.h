#ifndef CONF_H__
#define CONF_H__

#include <stdint.h>

#include <ncurses.h>
#include <regex.h>

#define CONF_BIND_QUIT "C-x C-c"
#define CONF_BIND_CHGFWD_FRAME "C-x b"
#define CONF_BIND_CHGBACK_FRAME "C-c b"
#define CONF_BIND_FOCUS_FRAME "C-c f"
#define CONF_BIND_KILL_FRAME "C-x k"
#define CONF_BIND_OPEN_FILE "C-x C-f"
#define CONF_BIND_SAVE_FILE "C-x C-s"
#define CONF_BIND_NAVFWD_CH "C-f"
#define CONF_BIND_NAVFWD_WORD "M-f"
#define CONF_BIND_NAVBACK_CH "C-b"
#define CONF_BIND_NAVBACK_WORD "M-b"
#define CONF_BIND_NAVDOWN "C-n"
#define CONF_BIND_NAVUP "C-p"
#define CONF_BIND_NAVLN_START "C-a"
#define CONF_BIND_NAVLN_END "C-e"
#define CONF_BIND_NAVGOTO "M-g M-g"
#define CONF_BIND_DEL_CH "<BACKSPC>"
#define CONF_BIND_DEL_WORD "M-<BACKSPC>"

#define CONF_GREET_TEXT \
	"welcome to medioed, the (medio)cre text (ed)itor\n" \
	"\n" \
	"this program is 100% free software, licensed under the GNU GPLv3\n" \
	"the source code is available at https://gitlab.com/tirimid/medioed\n" \
	"\n" \
	"copyleft (c) Fuck Copyright\n" \
	"\n" \
	"basic keybinds:\n" \
	"\tquit medioed              : " CONF_BIND_QUIT "\n" \
	"\tnavigate forward one char : " CONF_BIND_NAVFWD_CH "\n" \
	"\tnavigate forward one word : " CONF_BIND_NAVFWD_WORD "\n" \
	"\tnavigate back one char    : " CONF_BIND_NAVBACK_CH "\n" \
	"\tnavigate back one word    : " CONF_BIND_NAVBACK_WORD "\n" \
	"\tnavigate down one line    : " CONF_BIND_NAVDOWN "\n" \
	"\tnavigate up one line      : " CONF_BIND_NAVUP "\n" \
	"\topen file                 : " CONF_BIND_OPEN_FILE "\n" \
	"\tsave file                 : " CONF_BIND_SAVE_FILE "\n" \
	"\n" \
	"to customize visuals, controls, etc., edit `include/conf.h`\n"

#define CONF_TABSIZE 4
#define CONF_GUTTER_LEFT 1
#define CONF_GUTTER_RIGHT 1
#define CONF_BUFMOD_MARK "~~ (*)"

#define CONF_GNORM_BG COLOR_BLACK
#define CONF_GNORM_FG COLOR_YELLOW
#define CONF_GHIGHLIGHT_BG COLOR_YELLOW
#define CONF_GHIGHLIGHT_FG COLOR_BLACK

#define CONF_NORM_BG COLOR_BLACK
#define CONF_NORM_FG COLOR_WHITE
#define CONF_LINUM_BG COLOR_BLACK
#define CONF_LINUM_FG COLOR_YELLOW
#define CONF_CURSOR_BG COLOR_WHITE
#define CONF_CURSOR_FG COLOR_BLACK

enum highlight_scope {
	HIGHLIGHT_SCOPE_GLOBAL = 0,
	HIGHLIGHT_SCOPE_VISIBLE,
	HIGHLIGHT_SCOPE_LINE,
};

struct highlight {
	regex_t re;
	char const *re_str;
	char const *mode;
	int colpair;
	unsigned char scope; // see `enum highlight_scope`.
	uint8_t bg, fg;
};

struct margin {
	unsigned pos;
	int colpair;
	char ch;
	uint8_t fg, bg;
};

// to configure table variables, edit `src/conf.c`.
// tables are stored in a source file so that only one instance exists during
// program execution.
extern struct highlight conf_htab[];
extern size_t const conf_htab_size;

extern struct margin conf_mtab[];
extern size_t const conf_mtab_size;

extern int conf_gnorm, conf_ghighlight;
extern int conf_norm, conf_linum, conf_cursor;

int conf_init(void);
void conf_quit(void);

#endif
