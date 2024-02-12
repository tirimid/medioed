#include "hl/hl_c.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "draw.h"

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
#define A_MACRO_FG CONF_A_ACCENT_2_FG
#define A_MACRO_BG CONF_A_ACCENT_2_BG
#define A_SPECIAL_FG CONF_A_SPECIAL_FG
#define A_SPECIAL_BG CONF_A_SPECIAL_BG

#define SPECIAL L"+-()[].<>{}!~*&/%=?:|;,"

enum word_type {
	WT_MACRO,
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
	L"auto",
	L"break",
	L"case",
	L"char",
	L"const",
	L"continue",
	L"default",
	L"do",
	L"double",
	L"else",
	L"enum",
	L"extern",
	L"float",
	L"for",
	L"goto",
	L"if",
	L"inline",
	L"int",
	L"long",
	L"register",
	L"restrict",
	L"return",
	L"short",
	L"signed",
	L"sizeof",
	L"static",
	L"struct",
	L"switch",
	L"typedef",
	L"union",
	L"unsigned",
	L"void",
	L"volatile",
	L"while",
	L"_Bool",
	L"_Complex",
	L"_Imaginary",
};

int
hl_c_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
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
	while (j < len) {
		++j;
		if (src[j] == L'\\') {
			++j;
			while (iswspace(src[j]) && src[j] != L'\n')
				++j;
			if (src[j] == L'\n')
				++j;
		} else if (src[j] == L'\n')
			break;
	}

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
	size_t j = *i;
	while (j < len) {
		++j;
		if (src[j] == L'\\') {
			++j;
			continue;
		} else if (src[j] == L'"' || src[j] == L'\n')
			break;
	}
	
	*out_lb = *i;
	*out_ub = j + (j < len);
	*out_fg = A_STRING_FG;
	*out_bg = A_STRING_BG;
	
	return 0;
}

static int
hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 1;
	if (j < len && src[j] == L'\\')
		j += 2;
	else
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
		while (j < len && wcsncmp(src + j, L"*/", 2))
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
	enum word_type wt = WT_MACRO;
	
	size_t j = *i;
	while (j < len && (src[j] == L'_' || iswalnum(src[j]))) {
		if (iswlower(src[j]))
			wt = WT_BASIC;
		++j;
	}

	if (wt == WT_BASIC) {
		size_t k = j;
		while (k < len && iswspace(src[k]))
			++k;

		if (k < len && src[k] == L'(')
			wt = WT_FUNC;
	}

	for (size_t kw = 0; kw < ARRAY_SIZE(keywords); ++kw) {
		if (wcslen(keywords[kw]) != j - *i)
			continue;
		
		if (!wcsncmp(keywords[kw], src + *i, j - *i)) {
			wt = WT_KEYWORD;
			break;
		}
	}

	*out_lb = *i;
	*out_ub = j;

	switch (wt) {
	case WT_MACRO:
		*out_fg = A_MACRO_FG;
		*out_bg = A_MACRO_BG;
		break;
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
