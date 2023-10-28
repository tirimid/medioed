#include "hl/hl_c.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "draw.h"

#define A_PREPROC (A_YELLOW | A_BGOF(CONF_A_NORM))
#define A_KEYWORD (A_MAGENTA | A_BGOF(CONF_A_NORM) | A_BRIGHT)
#define A_COMMENT (A_RED | A_BGOF(CONF_A_NORM))
#define A_STRING (A_WHITE | A_BRED)
#define A_FUNC (A_WHITE | A_BGOF(CONF_A_NORM) | A_BRIGHT)
#define A_MACRO (A_MAGENTA | A_BGOF(CONF_A_NORM))
#define A_SPECIAL (A_WHITE | A_BGOF(CONF_A_NORM) | A_DIM)

#define SPECIAL "+-()[].<>{}!~*&/%=?:|;,"

enum wordtype {
	WT_MACRO,
	WT_FUNC,
	WT_KEYWORD,
	WT_BASIC,
};

static int hl_preproc(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);

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
          size_t *out_ub, uint16_t *out_a)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'#') {
			if (!hl_preproc(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} if (src[i] == L'"') {
			if (!hl_string(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'\'') {
			if (!hl_char(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (i < len - 1
		           && src[i] == L'/'
		           && (src[i + 1] == L'/' || src[i + 1] == L'*')) {
			if (!hl_comment(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (strchr(SPECIAL, src[i])) {
			if (!hl_special(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (iswalpha(src[i]) || src[i] == L'_') {
			if (!hl_word(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_preproc(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
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
	*out_a = A_PREPROC;
	
	return 0;
}

static int
hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
          size_t *out_ub, uint16_t *out_a)
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
	*out_a = A_STRING;
	
	return 0;
}

static int
hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	if (j < len && src[j] == L'\\')
		j += 2;
	else
		++j;

	if (j < len && src[j] == L'\'') {
		*out_lb = *i;
		*out_ub = j + 1;
		*out_a = A_STRING;
		
		return 0;
	}
	
	return 1;
}

static int
hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
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
	*out_a = A_COMMENT;
	
	return 0;
}

static int
hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	while (j < len && strchr(SPECIAL, src[j]))
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_a = A_SPECIAL;
	
	return 0;
}

static int
hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint16_t *out_a)
{
	enum wordtype wt = WT_MACRO;
	
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

	for (size_t kw = 0; kw < ARRAYSIZE(keywords); ++kw) {
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
		*out_a = A_MACRO;
		break;
	case WT_FUNC:
		*out_a = A_FUNC;
		break;
	case WT_KEYWORD:
		*out_a = A_KEYWORD;
		break;
	case WT_BASIC:
		*i = j - 1;
		return 1;
	}
	
	return 0;
}
