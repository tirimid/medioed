#include "hl/hl_cs.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "util.h"

#define A_PREPROC_FG CONF_A_ACCENT_3_FG
#define A_PREPROC_BG CONF_A_ACCENT_3_BG
#define A_KEYWORD_FG CONF_A_ACCENT_1_FG
#define A_KEYWORD_BG CONF_A_ACCENT_1_BG
#define A_COMMENT_FG CONF_A_COMMENT_FG
#define A_COMMENT_BG CONF_A_COMMENT_BG
#define A_STRING_FG CONF_A_STRING_FG
#define A_STRING_BG CONF_A_STRING_BG
#define A_FUNC_FG CONF_A_ACCENT_4_FG
#define A_FUNC_BG CONF_A_ACCENT_4_BG
#define A_SPECIAL_FG CONF_A_SPECIAL_FG
#define A_SPECIAL_BG CONF_A_SPECIAL_BG

#define SPECIAL L"+-()[].<>{}!~*&/%=?:|;,"

enum word_type {
	WT_FUNC,
	WT_KEYWORD,
	WT_BASIC,
};

static int hl_preproc(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

static wchar_t const *keywords[] = {
	L"abstract",
	L"add",
	L"alias",
	L"and",
	L"args",
	L"as",
	L"ascending",
	L"async",
	L"await",
	L"base",
	L"bool",
	L"break",
	L"by",
	L"byte",
	L"case",
	L"catch",
	L"char",
	L"checked",
	L"class",
	L"const",
	L"continue",
	L"decimal",
	L"default",
	L"delegate",
	L"descending",
	L"do",
	L"double",
	L"dynamic",
	L"else",
	L"enum",
	L"equals",
	L"event",
	L"explicit",
	L"extern",
	L"false",
	L"file",
	L"finally",
	L"fixed",
	L"float",
	L"for",
	L"foreach",
	L"from",
	L"get",
	L"global",
	L"goto",
	L"group",
	L"if",
	L"implicit",
	L"in",
	L"init",
	L"int",
	L"interface",
	L"internal",
	L"into",
	L"is",
	L"join",
	L"let",
	L"lock",
	L"long",
	L"managed",
	L"nameof",
	L"namespace",
	L"new",
	L"nint",
	L"not",
	L"notnull",
	L"nuint",
	L"null",
	L"object",
	L"on",
	L"operator",
	L"or",
	L"orderby",
	L"out",
	L"override",
	L"params",
	L"partial",
	L"private",
	L"protected",
	L"public",
	L"readonly",
	L"record",
	L"ref",
	L"remove",
	L"required",
	L"return",
	L"sbyte",
	L"scoped",
	L"sealed",
	L"select",
	L"set",
	L"short",
	L"sizeof",
	L"stackalloc",
	L"static",
	L"string",
	L"struct",
	L"switch",
	L"this",
	L"throw",
	L"true",
	L"try",
	L"typeof",
	L"uint",
	L"ulong",
	L"unchecked",
	L"unmanaged",
	L"unsafe",
	L"ushort",
	L"using",
	L"value",
	L"var",
	L"virtual",
	L"void",
	L"volatile",
	L"when",
	L"where",
	L"while",
	L"with",
	L"yield",
};

int
hl_cs_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'#') {
			if (!hl_preproc(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'"') {
			if (!hl_string(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'\'') {
			if (!hl_char(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (i + 1 < len
		           && src[i] == L'/'
		           && wcschr(L"/*", src[i + 1])) {
			if (!hl_comment(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (wcschr(SPECIAL, src[i])) {
			if (!hl_special(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (iswalpha(src[i]) || src[i] == L'_') {
			if (!hl_word(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_preproc(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i;
	while (j < len && src[j] != L'\n')
		++j;
	
	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_PREPROC_FG;
	*out_bg = A_PREPROC_BG;
	
	return 0;
}

static int
hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
          size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	// TODO: implement.
	return 1;
}

static int
hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 1;
	if (j < len && src[j] == L'\\') {
		++j;
		if (j < len && wcschr(L"xu", src[j])) {
			++j;
			while (j < len && iswxdigit(src[j]))
				++j;
		} else
			++j;
	} else
		++j;

	if (j < len && src[j] == L'\'') {
		*out_lb = *i;
		*out_ub = j + 1;
		*out_fg = A_STRING_FG;
		*out_bg = A_STRING_BG;
		
		return 0;
	}
	
	return 1;
}

static int
hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 2;
	if (src[*i + 1] == L'/') {
		while (j < len && src[j] != L'\n')
			++j;
	} else {
		while (j + 1 < len && wcsncmp(src + j, L"*/", 2))
			++j;
	}

	*out_lb = *i;
	*out_ub = j + 2 * (src[j] == '*');
	*out_fg = A_COMMENT_FG;
	*out_bg = A_COMMENT_BG;
	
	return 0;
}

static int
hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < len && wcschr(SPECIAL, src[j]))
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_SPECIAL_FG;
	*out_bg = A_SPECIAL_BG;
	
	return 0;
}

static int
hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	enum word_type wt = WT_BASIC;
	bool ident_pfx = *i > 0 && src[*i - 1] == L'@';
	
	size_t j = *i;
	while (j < len && (src[j] == L'_' || iswalnum(src[j])))
		++j;
	
	// check for (generic) function.
	// similarly to C++ highlight mode template checking, this is very
	// stupid and doesn't account for embedded comments.
	// however, this defect arguably doesn't even slightly matter.
	
	size_t k = j;
	while (k < len && iswspace(src[k]))
		++k;
	
	if (k < len && src[k] == L'<') {
		++k;
		for (unsigned nopen = 1; k < len && nopen > 0; ++k) {
			nopen += src[k] == L'<';
			nopen -= src[k] == L'>';
		}
	}
	
	while (k < len && iswspace(src[k]))
		++k;
	
	if (k < len && src[k] == L'(')
		wt = WT_FUNC;
	
	if (!ident_pfx) {
		for (size_t kw = 0; kw < ARRAY_SIZE(keywords); ++kw) {
			if (wcslen(keywords[kw]) != j - *i)
				continue;
			
			if (!wcsncmp(keywords[kw], src + *i, j - *i)) {
				wt = WT_KEYWORD;
				break;
			}
		}
	}
	
	*out_lb = *i;
	*out_ub = j;
	
	switch (wt) {
	case WT_FUNC:
		*out_fg = A_FUNC_FG;
		*out_bg = A_FUNC_BG;
		break;
	case WT_KEYWORD:
		*out_fg = A_KEYWORD_FG;
		*out_bg = A_KEYWORD_BG;
		break;
	case WT_BASIC:
		*i = j - 1;
		return 1;
	}
	
	return 0;
}
