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
#define GLOBAL_BIND_QUIT "^X ^C "
#define GLOBAL_BIND_CHGFWD_FRAME "^X b "
#define GLOBAL_BIND_CHGBACK_FRAME "^C b "
#define GLOBAL_BIND_FOCUS_FRAME "^C f "
#define GLOBAL_BIND_KILL_FRAME "^X k "
#define GLOBAL_BIND_OPEN_FILE "^X ^F "
#define GLOBAL_BIND_SAVE_FILE "^X ^S "
#define GLOBAL_BIND_NAVFWD_CH "^F "
#define GLOBAL_BIND_NAVFWD_WORD "^[ f "
#define GLOBAL_BIND_NAVBACK_CH "^B "
#define GLOBAL_BIND_NAVBACK_WORD "^[ b "
#define GLOBAL_BIND_NAVDOWN "^N "
#define GLOBAL_BIND_NAVUP "^P "
#define GLOBAL_BIND_NAVLN_START "^A "
#define GLOBAL_BIND_NAVLN_END "^E "
#define GLOBAL_BIND_NAVGOTO "^[ g ^[ g "
#define GLOBAL_BIND_DEL "^? "

// global textual configuration.
#define GLOBAL_GREET_TEXT \
	"welcome to medioed, the (medio)cre text (ed)itor\n" \
	"\n" \
	"this program is 100% free software, licensed under the GNU GPLv3\n" \
	"the source code is available at https://gitlab.com/tirimid/medioed\n" \
	"\n" \
	"copyleft (c) Fuck Copyright\n"

#endif
