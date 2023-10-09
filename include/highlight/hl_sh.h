#define HIGHLIGHT_SH(a, re_str_) \
	{ \
		.localmode = "sh", \
		.re_str = re_str_, \
		.attr = a, \
	},

#define A_COMMENT_SH (A_RED | A_BGOF(CONF_A_NORM))
#define A_STRING_SH (A_YELLOW | A_BGOF(CONF_A_NORM))
#define A_TRAILINGWS_SH (A_FGOF(CONF_A_NORM) | A_BGREEN)

HIGHLIGHT_SH(A_STRING_SH, L"\"(?:[^\"\\\\]|\\\\.)*\"")
HIGHLIGHT_SH(A_STRING_SH, L"'(?:[^'\\\\]|\\\\.)*'")
HIGHLIGHT_SH(A_COMMENT_SH, L"#.*")
HIGHLIGHT_SH(A_TRAILINGWS_SH, L"\\s+$")
