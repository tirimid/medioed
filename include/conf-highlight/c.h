#define KEYWORD(kw) "(" kw ")"

#define HIGHLIGHT(_bg, _fg, _re_str) \
	{ \
		.mode = "c", \
		.bg = _bg, \
		.fg = _fg, \
		.re_str = _re_str \
	},

HIGHLIGHT(COLOR_BLACK, COLOR_CYAN, "#\\s*[a-zA-Z_][a-zA-Z0-9_]*")
