#ifndef HL_SH_H__
#define HL_SH_H__

#include <ncurses.h>

#define HIGHLIGHT_SH(hl, re_str_) \
	{ \
		.localmode = "sh", \
		.bg = hl##_BG_SH, \
		.fg = hl##_FG_SH, \
		.attr = hl##_ATTR_SH, \
		.re_str = re_str_, \
	},

#define COMMENT_BG_SH CONF_NORM_BG
#define COMMENT_FG_SH COLOR_RED
#define COMMENT_ATTR_SH 0
#define STRING_BG_SH CONF_NORM_BG
#define STRING_FG_SH COLOR_YELLOW
#define STRING_ATTR_SH 0
#define TRAILINGWS_BG_SH COLOR_GREEN
#define TRAILINGWS_FG_SH CONF_NORM_FG
#define TRAILINGWS_ATTR_SH 0

HIGHLIGHT_SH(STRING, "\"(?:[^\"\\\\]|\\\\.)*\"")
HIGHLIGHT_SH(STRING, "'(?:[^'\\\\]|\\\\.)*'")
HIGHLIGHT_SH(COMMENT, "#.*")
HIGHLIGHT_SH(TRAILINGWS, "\\s+$")

#endif
