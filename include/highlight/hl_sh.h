#define HIGHLIGHT_SH(a, re_str_) \
	{ \
		.localmode = "sh", \
		.re_str = re_str_, \
		.attr = a, \
	},

#define KEYWORDS_SH \
	L"(^|(?<=[^A-Za-z0-9_]))(" \
	L"if|" \
	L"then|" \
	L"elif|" \
	L"else|" \
	L"fi|" \
	L"time|" \
	L"for|" \
	L"in|" \
	L"until|" \
	L"while|" \
	L"do|" \
	L"done|" \
	L"case|" \
	L"esac|" \
	L"coproc|" \
	L"select|" \
	L"function" \
	L")($|(?=[^A-Za-z0-9_]))"

#define BUILTINS_SH \
	L"(^|(?<=[^A-Za-z0-9_]))(" \
	L"break|" \
	L"cd|" \
	L"continue|" \
	L"eval|" \
	L"exec|" \
	L"exit|" \
	L"export|" \
	L"getopts|" \
	L"hash|" \
	L"pwd|" \
	L"readonly|" \
	L"return|" \
	L"shift|" \
	L"test|" \
	L"times|" \
	L"trap|" \
	L"umask|" \
	L"unset|" \
	L"bind|" \
	L"builtin|" \
	L"caller|" \
	L"command|" \
	L"declare|" \
	L"echo|" \
	L"enable|" \
	L"help|" \
	L"let|" \
	L"local|" \
	L"logout|" \
	L"mapfile|" \
	L"printf|" \
	L"read|" \
	L"readarray|" \
	L"source|" \
	L"type|" \
	L"typeset|" \
	L"ulimit|" \
	L"unalias" \
	L")($|(?=[^A-Za-z0-9_]))"

#define A_KEYWORD_SH (A_GREEN | A_BGOF(CONF_A_NORM))
#define A_BUILTIN_SH (A_MAGENTA | A_BGOF(CONF_A_NORM) | A_BRIGHT)
#define A_COMMENT_SH (A_RED | A_BGOF(CONF_A_NORM))
#define A_VAR_SH (A_CYAN | A_BGOF(CONF_A_NORM))
#define A_STRING_SH (A_YELLOW | A_BGOF(CONF_A_NORM))
#define A_TRAILINGWS_SH (A_FGOF(CONF_A_NORM) | A_BGREEN)

HIGHLIGHT_SH(A_KEYWORD_SH, KEYWORDS_SH)
HIGHLIGHT_SH(A_BUILTIN_SH, BUILTINS_SH)
HIGHLIGHT_SH(A_STRING_SH, L"\"(?:[^\"\\\\]|\\\\.)*\"")
HIGHLIGHT_SH(A_STRING_SH, L"'(?:[^'\\\\]|\\\\.)*'")
HIGHLIGHT_SH(A_VAR_SH, L"\\$(\\{(?:[^\\}\\\\]|\\\\.)*\\}|[a-zA-Z_][a-zA-Z0-9_]*)")
HIGHLIGHT_SH(A_COMMENT_SH, L"#.*")
HIGHLIGHT_SH(A_TRAILINGWS_SH, L"\\s+$")
