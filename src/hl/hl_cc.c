#include "hl/hl_cc.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include <unistd.h>

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
static int hl_rstring(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

static wchar_t const *keywords[] = {
	L"alignas",
	L"alignof",
	L"and",
	L"and_eq",
	L"asm",
	L"atomic_cancel",
	L"atomic_commit",
	L"atomic_noexcept",
	L"auto",
	L"bitand",
	L"bitor",
	L"bool",
	L"break",
	L"case",
	L"catch",
	L"char",
	L"char8_t",
	L"char16_t",
	L"char32_t",
	L"class",
	L"compl",
	L"concept",
	L"const",
	L"consteval",
	L"constexpr",
	L"constinit",
	L"const_cast",
	L"continue",
	L"co_await",
	L"co_return",
	L"co_yield",
	L"decltype",
	L"default",
	L"delete",
	L"do",
	L"double",
	L"dynamic_cast",
	L"else",
	L"enum",
	L"explicit",
	L"export",
	L"extern",
	L"false",
	L"float",
	L"for",
	L"friend",
	L"goto",
	L"if",
	L"inline",
	L"int",
	L"long",
	L"mutable",
	L"namespace",
	L"new",
	L"noexcept",
	L"not",
	L"not_eq",
	L"nullptr",
	L"operator",
	L"or",
	L"or_eq",
	L"private",
	L"protected",
	L"public",
	L"reflexpr",
	L"register",
	L"reinterpret_cast",
	L"requires",
	L"return",
	L"short",
	L"signed",
	L"sizeof",
	L"static",
	L"static_assert",
	L"static_cast",
	L"struct",
	L"switch",
	L"synchronized",
	L"template",
	L"this",
	L"thread_local",
	L"throw",
	L"true",
	L"try",
	L"typedef",
	L"typeid",
	L"typename",
	L"union",
	L"unsigned",
	L"using",
	L"virtual",
	L"void",
	L"volatile",
	L"wchar_t",
	L"while",
	L"xor",
	L"xor_eq",
};

int
hl_cc_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
		for (size_t i = off; i < len; ++i) {
		if (src[i] == L'#') {
			if (!hl_preproc(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (i + 2 < len && !wcsncmp(&src[i], L"R\"", 2)
		           || i + 3 < len && !wcsncmp(&src[i], L"LR\"", 3)
		           || i + 4 < len && !wcsncmp(&src[i], L"u8R\"", 4)
		           || i + 3 < len && !wcsncmp(&src[i], L"uR\"", 3)
		           || i + 3 < len && !wcsncmp(&src[i], L"UR\"", 3)) {
			if (!hl_rstring(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'"') {
			if (!hl_string(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'\'') {
			if (!hl_char(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (i + 1 < len
		           && src[i] == L'/'
		           && (src[i + 1] == L'/' || src[i + 1] == L'*')) {
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
hl_rstring(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	while (src[*i] != L'"')
		++*i;
	
	size_t j = *i + 1;
	while (j - *i <= 16 && src[j] != L'(' && src[j] != L'"')
		++j;
	
	// `d-char-seq` is max 16 chars, plus 3 for '"', ')', and 0.
	wchar_t term_seq[19] = {0};
	size_t d_char_seq_len = 0;
	if (src[j] == L'(') {
		d_char_seq_len = j - *i - 1;
		
		for (size_t k = 0; k < d_char_seq_len; ++k)
			term_seq[k + 1] = src[*i + k + 1];
		
		term_seq[0] = L')';
		term_seq[d_char_seq_len + 1] = L'"';
		term_seq[d_char_seq_len + 2] = 0;
	} else {
		term_seq[0] = L'"';
		term_seq[1] = 0;
	}
	
	while (j + d_char_seq_len + 2 < len
	       && wcsncmp(&src[j], term_seq, d_char_seq_len + 2)) {
		++j;
	}
	
	if (j + d_char_seq_len + 2 >= len)
		return 1;
	
	*out_lb = *i;
	*out_ub = j + d_char_seq_len + 2;
	*out_fg = A_STRING_FG;
	*out_bg = A_STRING_BG;
	
	return 0;
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
		
		// very dumb, does not do string or comment checking.
		// generally people don't put strings in function template
		// arguments anyway so that's probably fine.
		// at least, *I* never do that, and this is *my* editor.
		// fight me.
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
