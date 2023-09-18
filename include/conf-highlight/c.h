#define KEYWORD(kw) "(?<![a-zA-Z0-9_])" kw "(?![a-zA-Z0-0_])"

#define HIGHLIGHT(_bg, _fg, _re_str) \
	{ \
		.mode = "c", \
		.bg = _bg, \
		.fg = _fg, \
		.re_str = _re_str \
	},

#define PREPROC_BG CONF_NORM_BG
#define PREPROC_FG COLOR_CYAN
#define KEYWORD_BG CONF_NORM_BG
#define KEYWORD_FG COLOR_GREEN
#define COMMENT_BG CONF_NORM_BG
#define COMMENT_FG COLOR_RED
#define STRING_BG CONF_NORM_BG
#define STRING_FG COLOR_YELLOW

HIGHLIGHT(PREPROC_BG, PREPROC_FG, "#\\s*[a-zA-Z_][a-zA-Z0-9_]*")
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("auto"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("break"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("case"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("char"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("const"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("continue"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("default"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("do"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("double"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("else"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("enum"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("extern"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("float"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("for"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("goto"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("if"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("inline"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("int"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("long"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("register"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("restrict"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("return"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("short"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("signed"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("sizeof"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("static"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("struct"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("switch"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("typedef"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("union"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("unsigned"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("void"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("volatile"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("while"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("_Bool"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("_Complex"))
HIGHLIGHT(KEYWORD_BG, KEYWORD_FG, KEYWORD("_Imaginary"))
HIGHLIGHT(COMMENT_BG, COMMENT_FG, "//.*")
HIGHLIGHT(STRING_BG, STRING_FG, "\"(?:[^\"\\\\]|\\\\.)*\"")
