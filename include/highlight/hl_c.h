#ifndef HL_C_H__
#define HL_C_H__

#include <ncurses.h>

#define HIGHLIGHT(hl, re_str_) \
	{ \
		.localmode = "c", \
		.bg = hl##_BG, \
		.fg = hl##_FG, \
		.attr = hl##_ATTR, \
		.re_str = re_str_, \
	},

#define KEYWORDS \
	"(?<=[^A-Za-z0-9_])(" \
	"auto|" \
	"break|" \
	"case|" \
	"char|" \
	"const|" \
	"continue|" \
	"default|" \
	"do|" \
	"double|" \
	"else|" \
	"enum|" \
	"extern|" \
	"float|" \
	"for|" \
	"goto|" \
	"if|" \
	"inline|" \
	"int|" \
	"long|" \
	"register|" \
	"restrict|" \
	"return|" \
	"short|" \
	"signed|" \
	"sizeof|" \
	"static|" \
	"struct|" \
	"switch|" \
	"typedef|" \
	"union|" \
	"unsigned|" \
	"void|" \
	"volatile|" \
	"while|" \
	"_Bool|" \
	"_Complex|" \
	"_Imaginary" \
	")(?=[^A-Za-z0-9_])" \

#define PREPROC_BG CONF_NORM_BG
#define PREPROC_FG COLOR_CYAN
#define PREPROC_ATTR 0
#define KEYWORD_BG CONF_NORM_BG
#define KEYWORD_FG COLOR_GREEN
#define KEYWORD_ATTR 0
#define COMMENT_BG CONF_NORM_BG
#define COMMENT_FG COLOR_RED
#define COMMENT_ATTR 0
#define STRING_BG CONF_NORM_BG
#define STRING_FG COLOR_YELLOW
#define STRING_ATTR 0
#define FUNC_BG CONF_NORM_BG
#define FUNC_FG COLOR_MAGENTA
#define FUNC_ATTR A_BOLD
#define MACRO_BG CONF_NORM_BG
#define MACRO_FG COLOR_CYAN
#define MACRO_ATTR 0
#define SPECIAL_BG CONF_NORM_BG
#define SPECIAL_FG COLOR_BLUE
#define SPECIAL_ATTR 0
#define TRAILINGWS_BG COLOR_GREEN
#define TRAILINGWS_FG CONF_NORM_FG
#define TRAILINGWS_ATTR 0

HIGHLIGHT(FUNC, "(?<![a-zA-Z0-9_])[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()")
HIGHLIGHT(KEYWORD, KEYWORDS)
HIGHLIGHT(MACRO, "(?<![a-zA-Z0-9_])[A-Z_][A-Z0-9_]*(?![a-zA-Z0-9_])")
HIGHLIGHT(SPECIAL, "[\\+\\-\\(\\)\\[\\]\\.\\<\\>\\{\\}\\!\\~\\*\\&\\/\\%\\=\\?\\:\\|\\;\\,]")
HIGHLIGHT(PREPROC, "(\\n|^)\\s*\\K#\\s*[a-zA-Z_][a-zA-Z0-9_]*")
HIGHLIGHT(STRING, "(\\n|^)\\s*#\\s*include\\s*\\K<.*>")
HIGHLIGHT(STRING, "\"(?:[^\"\\\\]|\\\\.)*\"")
HIGHLIGHT(STRING, "'(?:[^'\\\\]|\\\\.)*'")
HIGHLIGHT(COMMENT, "//.*")
HIGHLIGHT(COMMENT, "\\/\\*(\\*(?!\\/)|[^*])*\\*\\/")
HIGHLIGHT(TRAILINGWS, "\\s+(\\n|$)")

#endif
