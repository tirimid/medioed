#include "hl/hl_rs.h"

#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "draw.h"
#include "util.h"

#define A_SPECIAL_FG CONF_A_SPECIAL_FG
#define A_SPECIAL_BG CONF_A_SPECIAL_BG
#define A_COMMENT_FG CONF_A_COMMENT_FG
#define A_COMMENT_BG CONF_A_COMMENT_BG
#define A_STRING_FG CONF_A_STRING_FG
#define A_STRING_BG CONF_A_STRING_BG
#define A_CONST_FG CONF_A_ACCENT_2_FG
#define A_CONST_BG CONF_A_ACCENT_2_BG
#define A_TYPE_FG CONF_A_ACCENT_2_FG
#define A_TYPE_BG CONF_A_ACCENT_2_BG
#define A_FUNC_FG CONF_A_ACCENT_4_FG
#define A_FUNC_BG CONF_A_ACCENT_4_BG
#define A_KEYWORD_FG CONF_A_ACCENT_1_FG
#define A_KEYWORD_BG CONF_A_ACCENT_1_BG

#define SPECIAL "!=%&*+,->./:;<@^|?#$(){}[]"

enum word_type {
	WT_CONST,
	WT_TYPE,
	WT_FUNC,
	WT_KEYWORD,
	WT_BASIC,
};

static int hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_rstring(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_quote(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_ln_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_blk_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

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
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'"') {
			if (!hl_string(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (i + 1 < len
		           && src[i] == L'r'
		           && wcschr(L"\"#", src[i + 1])) {
			if (!hl_rstring(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'\'') {
			if (!hl_quote(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (i + 1 < len
		           && src[i] == L'/'
		           && wcschr(L"/*", src[i + 1])) {
			switch (src[i + 1]) {
			case L'/':
				if (!hl_ln_comment(src, len, &i, out_lb, out_ub, out_fg, out_bg))
					return 0;
				break;
			case L'*':
				if (!hl_blk_comment(src, len, &i, out_lb, out_ub, out_fg, out_bg))
					return 0;
				break;
			default:
				break;
			}
		} else if (strchr(SPECIAL, src[i])) {
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
hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
          size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
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
	*out_fg = A_STRING_FG;
	*out_bg = A_STRING_BG;
	
	return 0;
}

static int
hl_rstring(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
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
			*out_fg = A_STRING_FG;
			*out_bg = A_STRING_BG;
			
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
         size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 1;
	if (j < len && src[j] == L'\\')
		j += 2;
	else
		++j;

	*out_lb = *i;
	if (j < len && src[j] == L'\'') {
		*out_ub = j + 1;
		*out_fg = A_STRING_FG;
		*out_bg = A_STRING_BG;
	} else {
		*out_ub = *i + 1;
		*out_fg = A_SPECIAL_FG;
		*out_bg = A_SPECIAL_BG;
	}

	return 0;
}

static int
hl_ln_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
              size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 2;
	while (j < len && src[j] != L'\n')
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_COMMENT_FG;
	*out_bg = A_COMMENT_BG;
	
	return 0;
}

static int
hl_blk_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
               size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
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
			*out_fg = A_COMMENT_FG;
			*out_bg = A_COMMENT_BG;
			
			return 0;
		}

		++j;
	}
	
	return 1;
}

static int
hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < len && strchr(SPECIAL, src[j]))
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
		*out_fg = A_CONST_FG;
		*out_bg = A_CONST_BG;
		break;
	case WT_TYPE:
		*out_fg = A_TYPE_FG;
		*out_bg = A_TYPE_BG;
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
