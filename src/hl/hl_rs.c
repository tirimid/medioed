#include "hl/hl_rs.h"

#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "draw.h"
#include "util.h"

#define A_SPECIAL (A_WHITE | A_BG_OF(CONF_A_NORM) | A_DIM)
#define A_COMMENT (A_RED | A_BG_OF(CONF_A_NORM))
#define A_STRING (A_WHITE | A_BRED)
#define A_CONST (A_MAGENTA | A_BG_OF(CONF_A_NORM))
#define A_TYPE (A_MAGENTA | A_BG_OF(CONF_A_NORM))
#define A_FUNC (A_WHITE | A_BG_OF(CONF_A_NORM) | A_BRIGHT)
#define A_KEYWORD (A_MAGENTA | A_BG_OF(CONF_A_NORM) | A_BRIGHT)

#define SPECIAL "!=%&*+,->./:;<@^|?#$(){}[]"

enum word_type {
	WT_CONST,
	WT_TYPE,
	WT_FUNC,
	WT_KEYWORD,
	WT_BASIC,
};

static int hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_rstring(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_quote(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_ln_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_blk_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);

static wchar_t const *keywords[] = {
	L"as",
	L"async",
	L"await",
	L"break",
	L"const",
	L"continue",
	L"crate",
	L"dyn",
	L"else",
	L"enum",
	L"extern",
	L"false",
	L"fn",
	L"for",
	L"if",
	L"impl",
	L"in",
	L"let",
	L"loop",
	L"match",
	L"mod",
	L"move",
	L"mut",
	L"pub",
	L"ref",
	L"return",
	L"Self",
	L"self",
	L"static",
	L"struct",
	L"super",
	L"trait",
	L"type",
	L"union",
	L"unsafe",
	L"use",
	L"where",
	L"while",
	L"abstract",
	L"become",
	L"box",
	L"do",
	L"final",
	L"macro",
	L"override",
	L"priv",
	L"try",
	L"typeof",
	L"unsized",
	L"virtual",
	L"yield",
};

int
hl_rs_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'"') {
			if (!hl_string(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (i + 1 < len
		           && src[i] == L'r'
		           && wcschr(L"\"#", src[i + 1])) {
			if (!hl_rstring(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'\'') {
			if (!hl_quote(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (i + 1 < len
		           && src[i] == L'/'
		           && wcschr(L"/*", src[i + 1])) {
			switch (src[i + 1]) {
			case L'/':
				if (!hl_ln_comment(src, len, &i, out_lb, out_ub, out_a))
					return 0;
				break;
			case L'*':
				if (!hl_blk_comment(src, len, &i, out_lb, out_ub, out_a))
					return 0;
				break;
			default:
				break;
			}
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
hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
          size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i;
	while (j < len) {
		++j;
		if (src[j] == L'\\') {
			++j;
			continue;
		} else if (src[j] == L'"')
			break;
	}
	
	*out_lb = *i;
	*out_ub = j + (j < len);
	*out_a = A_STRING;
	
	return 0;
}

static int
hl_rstring(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	while (j < len && src[j] == L'#')
		++j;
	if (j >= len || src[j] != L'"')
		return 1;
	j += j < len;

	unsigned nhash = (j - 1) - (*i + 1);

	wchar_t *search = malloc(sizeof(wchar_t) * (1 + nhash)); 
	search[0] = L'"';
	for (unsigned i = 1; i < 1 + nhash; ++i)
		search[i] = L'#';
	
	while (j < len) {
		if (!wcsncmp(&src[j], search, 1 + nhash)) {
			*out_lb = *i;
			*out_ub = j + nhash + 1;
			*out_a = A_STRING;
			
			free(search);
			return 0;
		}
		++j;
	}

	free(search);
	return 1;
}

static int
hl_quote(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	if (j < len && src[j] == L'\\')
		j += 2;
	else
		++j;

	*out_lb = *i;
	if (j < len && src[j] == L'\'') {
		*out_ub = j + 1;
		*out_a = A_STRING;
	} else {
		*out_ub = *i + 1;
		*out_a = A_SPECIAL;
	}

	return 0;
}

static int
hl_ln_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
              size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 2;
	while (j < len && src[j] != L'\n')
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_a = A_COMMENT;
	
	return 0;
}

static int
hl_blk_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
               size_t *out_ub, uint16_t *out_a)
{
	unsigned nopen = 1;
	size_t j = *i + 2;
	while (j + 1 < len) {
		if (!wcsncmp(&src[j], L"*/", 2)) {
			--nopen;
			++j;
		} else if (!wcsncmp(&src[j], L"/*", 2)) {
			++nopen;
			++j;
		}

		if (nopen == 0) {
			*out_lb = *i;
			*out_ub = j + 1;
			*out_a = A_COMMENT;
			
			return 0;
		}

		++j;
	}
	
	return 1;
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
	enum word_type wt = WT_BASIC;

	size_t j = *i;
	unsigned nunder = 0, nlower = 0, nupper = 0;
	while (j < len && (src[j] == L'_' || iswalnum(src[j]))) {
		if (src[j] == L'_')
			++nunder;
		else if (iswlower(src[j]))
			++nlower;
		else if (iswupper(src[j]))
			++nupper;
		++j;
	}

	if (!nunder && nlower && nupper)
		wt = WT_TYPE;
	else if (!nlower && nupper)
		wt = WT_CONST;
	else
		wt = WT_BASIC;

	if (j - *i == 1 && src[*i] == L'_')
		wt = WT_BASIC;

	if (wt == WT_BASIC) {
		size_t k = j;

		// whitespace scenario is not handled here like in C highlight
		// mode.
		// this is because seeing something like:
		// ```
		// function_call (...); // notice the space between `l` and `(`.
		// ```
		// is common enough in C that it merits inclusion in the
		// highlight handling, but noone really ever does that in Rust,
		// so it is not handled.
		
		if (k + 2 < len && !wcsncmp(&src[k], L"::<", 3)) {
			k += 3;
			unsigned nopen = 1;
			while (k < len && nopen > 0) {
				nopen += src[k] == L'<';
				nopen -= src[k] == L'>';
				++k;
			}
		}

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
	case WT_CONST:
		*out_a = A_CONST;
		break;
	case WT_TYPE:
		*out_a = A_TYPE;
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
