#ifndef HL_C_H__
#define HL_C_H__

#include <ncurses.h>

#define HIGHLIGHT_C(hl, re_str_) \
	{ \
		.localmode = "c", \
		.bg = hl##_BG_C, \
		.fg = hl##_FG_C, \
		.attr = hl##_ATTR_C, \
		.re_str = re_str_, \
	},

#define KEYWORDS_C \
	"(^|(?<=[^A-Za-z0-9_]))(" \
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
	")($|(?=[^A-Za-z0-9_]))"

#define PREPROC_BG_C CONF_NORM_BG
#define PREPROC_FG_C COLOR_CYAN
#define PREPROC_ATTR_C 0
#define KEYWORD_BG_C CONF_NORM_BG
#define KEYWORD_FG_C COLOR_GREEN
#define KEYWORD_ATTR_C 0
#define COMMENT_BG_C CONF_NORM_BG
#define COMMENT_FG_C COLOR_RED
#define COMMENT_ATTR_C 0
#define STRING_BG_C CONF_NORM_BG
#define STRING_FG_C COLOR_YELLOW
#define STRING_ATTR_C 0
#define FUNC_BG_C CONF_NORM_BG
#define FUNC_FG_C COLOR_MAGENTA
#define FUNC_ATTR_C A_BOLD
#define MACRO_BG_C CONF_NORM_BG
#define MACRO_FG_C COLOR_CYAN
#define MACRO_ATTR_C 0
#define SPECIAL_BG_C CONF_NORM_BG
#define SPECIAL_FG_C COLOR_BLUE
#define SPECIAL_ATTR_C 0
#define TRAILINGWS_BG_C COLOR_GREEN
#define TRAILINGWS_FG_C CONF_NORM_FG
#define TRAILINGWS_ATTR_C 0

HIGHLIGHT_C(FUNC, "(?<![a-zA-Z0-9_])[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()")
HIGHLIGHT_C(KEYWORD, KEYWORDS_C)
HIGHLIGHT_C(MACRO, "(?<![a-zA-Z0-9_])[A-Z_][A-Z0-9_]*(?![a-zA-Z0-9_])")
HIGHLIGHT_C(SPECIAL, "[\\+\\-\\(\\)\\[\\]\\.\\<\\>\\{\\}\\!\\~\\*\\&\\/\\%\\=\\?\\:\\|\\;\\,]")
HIGHLIGHT_C(PREPROC, "^\\s*\\K#\\s*[a-zA-Z_][a-zA-Z0-9_]*")
HIGHLIGHT_C(STRING, "^\\s*#\\s*include\\s*\\K<.*>")
HIGHLIGHT_C(STRING, "\"(?:[^\"\\\\\\n]|\\\\.)*\"")
HIGHLIGHT_C(STRING, "'(?:[^'\\\\\\n]|\\\\.)*'")
HIGHLIGHT_C(COMMENT, "//.*")
HIGHLIGHT_C(COMMENT, "\\/\\*(\\*(?!\\/)|[^*])*\\*\\/")
HIGHLIGHT_C(TRAILINGWS, "\\s+$")

#endif
