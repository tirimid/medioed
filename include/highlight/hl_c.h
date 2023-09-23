#ifndef HL_C_H__
#define HL_C_H__

#include <ncurses.h>

#define KEYWORD(kw) "(?<![a-zA-Z0-9_])" kw "(?![a-zA-Z0-0_])"

#define HIGHLIGHT(bg_, fg_, attr_, re_str_) \
	{ \
		.localmode = "c", \
		.bg = bg_, \
		.fg = fg_, \
		.attr = attr_, \
		.re_str = re_str_ \
	},

#define KEYWORDS \
	KEYWORD("auto") "|" \
	KEYWORD("break") "|" \
	KEYWORD("case") "|" \
	KEYWORD("char") "|" \
	KEYWORD("const") "|" \
	KEYWORD("continue") "|" \
	KEYWORD("default") "|" \
	KEYWORD("do") "|" \
	KEYWORD("double") "|" \
	KEYWORD("else") "|" \
	KEYWORD("enum") "|" \
	KEYWORD("extern") "|" \
	KEYWORD("float") "|" \
	KEYWORD("for") "|" \
	KEYWORD("goto") "|" \
	KEYWORD("if") "|" \
	KEYWORD("inline") "|" \
	KEYWORD("int") "|" \
	KEYWORD("long") "|" \
	KEYWORD("register") "|" \
	KEYWORD("restrict") "|" \
	KEYWORD("return") "|" \
	KEYWORD("short") "|" \
	KEYWORD("signed") "|" \
	KEYWORD("sizeof") "|" \
	KEYWORD("static") "|" \
	KEYWORD("struct") "|" \
	KEYWORD("switch") "|" \
	KEYWORD("typedef") "|" \
	KEYWORD("union") "|" \
	KEYWORD("unsigned") "|" \
	KEYWORD("void") "|" \
	KEYWORD("volatile") "|" \
	KEYWORD("while") "|" \
	KEYWORD("_Bool") "|" \
	KEYWORD("_Complex") "|" \
	KEYWORD("_Imaginary")

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

HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD_ATTR, KEYWORDS)
HIGHLIGHT(FUNC_BG, FUNC_FG, FUNC_ATTR, "(?<![a-zA-Z0-9_])[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()")
HIGHLIGHT(MACRO_BG, MACRO_FG, MACRO_ATTR, "(?<![a-zA-Z0-9_])[A-Z_][A-Z0-9_]*(?![a-zA-Z0-9_])")
HIGHLIGHT(SPECIAL_BG, SPECIAL_FG, SPECIAL_ATTR, "[\\+\\-\\(\\)\\[\\]\\.\\<\\>\\{\\}\\!\\~\\*\\&\\/\\%\\=\\?\\:\\|\\;\\,]")
HIGHLIGHT(PREPROC_BG, PREPROC_FG, PREPROC_ATTR, "#\\s*[a-zA-Z_][a-zA-Z0-9_]*")
HIGHLIGHT(STRING_BG, STRING_FG, STRING_ATTR, "#\\s*include\\s*\\K<.*>")
HIGHLIGHT(STRING_BG, STRING_FG, STRING_ATTR, "\"(?:[^\"\\\\]|\\\\.)*\"")
HIGHLIGHT(STRING_BG, STRING_FG, STRING_ATTR, "'(?:[^'\\\\]|\\\\.)*'")
HIGHLIGHT(COMMENT_BG, COMMENT_FG, COMMENT_ATTR, "//.*")
HIGHLIGHT(COMMENT_BG, COMMENT_FG, COMMENT_ATTR, "\\/\\*(\\*(?!\\/)|[^*])*\\*\\/")

#endif
