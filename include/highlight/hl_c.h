#ifndef HL_C_H__
#define HL_C_H__

#define KEYWORD(kw) "(?<![a-zA-Z0-9_])" kw "(?![a-zA-Z0-0_])"

#define HIGHLIGHT(_bg, _fg, _re_str) \
	{ \
		.mode = "c", \
		.bg = _bg, \
		.fg = _fg, \
		.re_str = _re_str \
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
#define KEYWORD_BG CONF_NORM_BG
#define KEYWORD_FG COLOR_GREEN
#define COMMENT_BG CONF_NORM_BG
#define COMMENT_FG COLOR_RED
#define STRING_BG CONF_NORM_BG
#define STRING_FG COLOR_YELLOW
#define FUNC_BG CONF_NORM_BG
#define FUNC_FG COLOR_MAGENTA
#define MACRO_BG CONF_NORM_BG
#define MACRO_FG COLOR_CYAN
#define SPECIAL_BG CONF_NORM_BG
#define SPECIAL_FG COLOR_BLUE

HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORDS)
HIGHLIGHT(FUNC_BG, FUNC_FG, "(?<![a-zA-Z0-9_])[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()")
HIGHLIGHT(MACRO_BG, MACRO_FG, "(?<![a-zA-Z0-9_])[A-Z_][A-Z0-9_]*(?![a-zA-Z0-9_])")
HIGHLIGHT(SPECIAL_BG, SPECIAL_FG, "[\\+\\-\\(\\)\\[\\]\\.\\<\\>\\{\\}\\!\\~\\*\\&\\/\\%\\=\\?\\:\\|\\;\\,]")
HIGHLIGHT(PREPROC_BG, PREPROC_FG, "#\\s*[a-zA-Z_][a-zA-Z0-9_]*")
HIGHLIGHT(STRING_BG, STRING_FG, "#\\s*include\\s*\\K<.*>")
HIGHLIGHT(STRING_BG, STRING_FG, "\"(?:[^\"\\\\]|\\\\.)*\"")
HIGHLIGHT(STRING_BG, STRING_FG, "'(?:[^'\\\\]|\\\\.)*'")
HIGHLIGHT(COMMENT_BG, COMMENT_FG, "//.*")
HIGHLIGHT(COMMENT_BG, COMMENT_FG, "\\/\\*(\\*(?!\\/)|[^*])*\\*\\/")

#endif
