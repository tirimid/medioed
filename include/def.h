#ifndef DEF_H__
#define DEF_H__

#include <ncurses.h>

// global render configuration.
#define GLOBAL_NORM_PAIR 1
#define GLOBAL_HIGHLIGHT_PAIR 2
#define GLOBAL_NORM_BG COLOR_BLACK
#define GLOBAL_NORM_FG COLOR_YELLOW
#define GLOBAL_HIGHLIGHT_BG COLOR_YELLOW
#define GLOBAL_HIGHLIGHT_FG COLOR_BLACK

// global bind configuration.
#define GLOBAL_BIND_QUIT "C-x C-c"
#define GLOBAL_BIND_CHGFWD_FRAME "C-x b"
#define GLOBAL_BIND_CHGBACK_FRAME "C-c b"
#define GLOBAL_BIND_FOCUS_FRAME "C-c f"
#define GLOBAL_BIND_KILL_FRAME "C-x k"
#define GLOBAL_BIND_OPEN_FILE "C-x C-f"
#define GLOBAL_BIND_SAVE_FILE "C-x C-s"
#define GLOBAL_BIND_NAVFWD_CH "C-f"
#define GLOBAL_BIND_NAVFWD_WORD "M-f"
#define GLOBAL_BIND_NAVBACK_CH "C-b"
#define GLOBAL_BIND_NAVBACK_WORD "M-b"
#define GLOBAL_BIND_NAVDOWN "C-n"
#define GLOBAL_BIND_NAVUP "C-p"
#define GLOBAL_BIND_NAVLN_START "C-a"
#define GLOBAL_BIND_NAVLN_END "C-e"
#define GLOBAL_BIND_NAVGOTO "M-g M-g"
#define GLOBAL_BIND_DEL "<BACKSPC>"

// global textual configuration.
#define GLOBAL_GREET_TEXT \
	"welcome to medioed, the (medio)cre text (ed)itor\n" \
	"\n" \
	"this program is 100% free software, licensed under the GNU GPLv3\n" \
	"the source code is available at https://gitlab.com/tirimid/medioed\n" \
	"\n" \
	"copyleft (c) Fuck Copyright\n" \
	"\n" \
	"basic keybinds:\n" \
	"\tquit medioed              : " GLOBAL_BIND_QUIT "\n" \
	"\tnavigate forward one char : " GLOBAL_BIND_NAVFWD_CH "\n" \
	"\tnavigate forward one word : " GLOBAL_BIND_NAVFWD_WORD "\n" \
	"\tnavigate back one char    : " GLOBAL_BIND_NAVBACK_CH "\n" \
	"\tnavigate back one word    : " GLOBAL_BIND_NAVBACK_WORD "\n" \
	"\tnavigate down one line    : " GLOBAL_BIND_NAVDOWN "\n" \
	"\tnavigate up one line      : " GLOBAL_BIND_NAVUP "\n" \
	"\topen file                 : " GLOBAL_BIND_OPEN_FILE "\n" \
	"\tsave file                 : " GLOBAL_BIND_SAVE_FILE "\n" \
	"\n" \
	"more useful keybinds can be found in `include/def.h` of the source code\n"

#endif
