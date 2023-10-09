#define HIGHLIGHT_C(a, re_str_) \
	{ \
		.localmode = "c", \
		.re_str = re_str_, \
		.attr = a, \
	},

#define KEYWORDS_C \
	L"(^|(?<=[^A-Za-z0-9_]))(" \
	L"auto|" \
	L"break|" \
	L"case|" \
	L"char|" \
	L"const|" \
	L"continue|" \
	L"default|" \
	L"do|" \
	L"double|" \
	L"else|" \
	L"enum|" \
	L"extern|" \
	L"float|" \
	L"for|" \
	L"goto|" \
	L"if|" \
	L"inline|" \
	L"int|" \
	L"long|" \
	L"register|" \
	L"restrict|" \
	L"return|" \
	L"short|" \
	L"signed|" \
	L"sizeof|" \
	L"static|" \
	L"struct|" \
	L"switch|" \
	L"typedef|" \
	L"union|" \
	L"unsigned|" \
	L"void|" \
	L"volatile|" \
	L"while|" \
	L"_Bool|" \
	L"_Complex|" \
	L"_Imaginary" \
	L")($|(?=[^A-Za-z0-9_]))"

#define A_PREPROC_C (A_CYAN | A_BGOF(CONF_A_NORM))
#define A_KEYWORD_C (A_GREEN | A_BGOF(CONF_A_NORM))
#define A_COMMENT_C (A_RED | A_BGOF(CONF_A_NORM))
#define A_STRING_C (A_YELLOW | A_BGOF(CONF_A_NORM))
#define A_FUNC_C (A_BRIGHT | A_MAGENTA | A_BGOF(CONF_A_NORM))
#define A_MACRO_C (A_CYAN | A_BGOF(CONF_A_NORM))
#define A_SPECIAL_C (A_BLUE | A_BGOF(CONF_A_NORM))
#define A_TRAILINGWS_C (A_FGOF(CONF_A_NORM) | A_BGREEN)

HIGHLIGHT_C(A_FUNC_C, L"(?<![a-zA-Z0-9_])[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()")
HIGHLIGHT_C(A_KEYWORD_C, KEYWORDS_C)
HIGHLIGHT_C(A_MACRO_C, L"(?<![a-zA-Z0-9_])[A-Z_][A-Z0-9_]*(?![a-zA-Z0-9_])")
HIGHLIGHT_C(A_SPECIAL_C, L"[\\+\\-\\(\\)\\[\\]\\.\\<\\>\\{\\}\\!\\~\\*\\&\\/\\%\\=\\?\\:\\|\\;\\,]")
HIGHLIGHT_C(A_PREPROC_C, L"^\\s*\\K#\\s*[a-zA-Z_][a-zA-Z0-9_]*")
HIGHLIGHT_C(A_STRING_C, L"^\\s*#\\s*include\\s*\\K<.*>")
HIGHLIGHT_C(A_STRING_C, L"\"(?:[^\"\\\\\\n]|\\\\.)*\"")
HIGHLIGHT_C(A_STRING_C, L"'(?:[^'\\\\\\n]|\\\\.)*'")
HIGHLIGHT_C(A_COMMENT_C, L"//.*")
HIGHLIGHT_C(A_COMMENT_C, L"\\/\\*(\\*(?!\\/)|[^*])*\\*\\/")
HIGHLIGHT_C(A_TRAILINGWS_C, L"\\s+$")
