#include "hl/hl_cc.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

#include <unistd.h>

#include "conf.h"

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

#define SPECIAL L"+-()[].<>{}!~*&/%=?:|;,^"

enum word_type
{
	WT_MACRO,
	WT_FUNC,
	WT_KEYWORD,
	WT_BASIC,
};

static int hl_preproc(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_string(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_char(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_rstring(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_comment(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

static wchar_t const *keywords[] =
{
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
hl_cc_find(struct buf const *buf,
           size_t off,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	wchar_t cmp_buf[20]; // max rstr compare length (19) plus 1.
	
	for (size_t i = off; i < buf->size; ++i)
	{
		if (buf_get_wch(buf, i) == L'#')
		{
			if (!hl_preproc(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (i + 2 < buf->size && !wcscmp(buf_get_wstr(buf, cmp_buf, i, 3), L"R\"")
		         || i + 3 < buf->size && !wcscmp(buf_get_wstr(buf, cmp_buf, i, 4), L"LR\"")
		         || i + 4 < buf->size && !wcscmp(buf_get_wstr(buf, cmp_buf, i, 5), L"u8R\"")
		         || i + 3 < buf->size && !wcscmp(buf_get_wstr(buf, cmp_buf, i, 4), L"uR\"")
		         || i + 3 < buf->size && !wcscmp(buf_get_wstr(buf, cmp_buf, i, 4), L"UR\""))
		{
			if (!hl_rstring(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (buf_get_wch(buf, i) == L'"')
		{
			if (!hl_string(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (buf_get_wch(buf, i) == L'\'')
		{
			if (!hl_char(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (i + 1 < buf->size
		         && buf_get_wch(buf, i) == L'/'
		         && wcschr(L"/*", buf_get_wch(buf, i + 1)))
		{
			if (!hl_comment(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (wcschr(SPECIAL, buf_get_wch(buf, i)))
		{
			if (!hl_special(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (iswalpha(buf_get_wch(buf, i))
		         || buf_get_wch(buf, i) == L'_')
		{
			if (!hl_word(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_preproc(struct buf const *buf,
           size_t *i,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	size_t j = *i;
	while (j < buf->size)
	{
		++j;
		if (buf_get_wch(buf, j) == L'\\')
		{
			++j;
			while (iswspace(buf_get_wch(buf, j))
			       && buf_get_wch(buf, j) != L'\n')
			{
				++j;
			}
			if (buf_get_wch(buf, j) == L'\n')
				++j;
		}
		else if (buf_get_wch(buf, j) == L'\n')
			break;
	}

	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_PREPROC_FG;
	*out_bg = A_PREPROC_BG;
	
	return 0;
}

static int
hl_string(struct buf const *buf,
          size_t *i,
          size_t *out_lb,
          size_t *out_ub,
          uint8_t *out_fg,
          uint8_t *out_bg)
{
	size_t j = *i;
	while (j < buf->size)
	{
		++j;
		if (buf_get_wch(buf, j) == L'\\')
		{
			++j;
			continue;
		}
		else if (wcschr(L"\"\n", buf_get_wch(buf, j)))
			break;
	}
	
	*out_lb = *i;
	*out_ub = j + (j < buf->size);
	*out_fg = A_STRING_FG;
	*out_bg = A_STRING_BG;
	
	return 0;
}

static int
hl_char(struct buf const *buf,
        size_t *i,
        size_t *out_lb,
        size_t *out_ub,
        uint8_t *out_fg,
        uint8_t *out_bg)
{
	// C++23 escape sequences not handled here.
	// they should be trivial to add if actually needed.
	
	size_t j = *i + 1;
	if (j < buf->size && buf_get_wch(buf, j) == L'\\')
	{
		++j;
		
		if (j < buf->size && wcschr(L"01234567", buf_get_wch(buf, j)))
		{
			// octal escape sequences.
			while (j < buf->size
			       && wcschr(L"01234567", buf_get_wch(buf, j)))
			{
				++j;
			}
		}
		else if (j < buf->size
		         && wcschr(L"xuU", buf_get_wch(buf, j)))
		{
			// hexadecimal-based escape seuences.
			++j;
			while (j < buf->size && iswxdigit(buf_get_wch(buf, j)))
				++j;
		}
		else
			++j;
	}
	else
		++j;

	if (j < buf->size && buf_get_wch(buf, j) == L'\'')
	{
		*out_lb = *i;
		*out_ub = j + 1;
		*out_fg = A_STRING_FG;
		*out_bg = A_STRING_BG;
		
		return 0;
	}
	
	return 1;
}

static int
hl_rstring(struct buf const *buf,
           size_t *i,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	while (buf_get_wch(buf, *i) != L'"')
		++*i;
	
	size_t j = *i + 1;
	while (j < buf->size
	       && j - *i <= 16
	       && !wcschr(L"(\"", buf_get_wch(buf, j)))
	{
		++j;
	}
	
	if (j >= buf->size || buf_get_wch(buf, j) != L'(')
		return 1;
	
	// `d-char-seq` is max 16 chars, plus 3 for '"', ')', and null.
	wchar_t term_seq[19] = {0};
	size_t d_char_seq_len = j - *i - 1;
	
	for (size_t k = 0; k < d_char_seq_len; ++k)
		term_seq[k + 1] = buf_get_wch(buf, *i + k + 1);
	term_seq[0] = L')';
	term_seq[d_char_seq_len + 1] = L'"';
	
	while (j + d_char_seq_len + 2 < buf->size)
	{
		wchar_t cmp_buf[19];
		buf_get_wstr(buf, cmp_buf, j, d_char_seq_len + 3);
		if (!wcscmp(cmp_buf, term_seq))
			break;
		++j;
	}
	
	if (j + d_char_seq_len + 2 >= buf->size)
		return 1;
	
	*out_lb = *i;
	*out_ub = j + d_char_seq_len + 2;
	*out_fg = A_STRING_FG;
	*out_bg = A_STRING_BG;
	
	return 0;
}

static int
hl_comment(struct buf const *buf,
           size_t *i,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	size_t j = *i + 2;
	if (buf_get_wch(buf, *i + 1) == L'/')
	{
		while (j < buf->size && buf_get_wch(buf, j) != L'\n')
			++j;
	}
	else
	{
		wchar_t cmp[3];
		while (j + 1 < buf->size
		       && wcscmp(buf_get_wstr(buf, cmp, j, 3), L"*/"))
		{
			++j;
		}
	}
	
	*out_lb = *i;
	*out_ub = j + 2 * (buf_get_wch(buf, j) == '*');
	*out_fg = A_COMMENT_FG;
	*out_bg = A_COMMENT_BG;
	
	return 0;
}

static int
hl_special(struct buf const *buf,
           size_t *i,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < buf->size && wcschr(SPECIAL, buf_get_wch(buf, j)))
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_SPECIAL_FG;
	*out_bg = A_SPECIAL_BG;
	
	return 0;
}

static int
hl_word(struct buf const *buf,
        size_t *i,
        size_t *out_lb,
        size_t *out_ub,
        uint8_t *out_fg,
        uint8_t *out_bg)
{
	enum word_type wt = WT_MACRO;
	
	size_t j = *i;
	while (j < buf->size)
	{
		if (buf_get_wch(buf, j) != L'_'
		    && !iswalnum(buf_get_wch(buf, j)))
		{
			break;
		}
		
		if (iswlower(buf_get_wch(buf, j)))
			wt = WT_BASIC;
		
		++j;
	}

	if (wt == WT_BASIC)
	{
		size_t k = j;
		while (k < buf->size && iswspace(buf_get_wch(buf, k)))
			++k;
		
		// very dumb, does not do string or comment checking.
		// generally people don't put strings in function template
		// arguments anyway so that's probably fine.
		// at least, *I* never do that, and this is *my* editor.
		// fight me.
		if (k < buf->size && buf_get_wch(buf, k) == L'<')
		{
			++k;
			for (unsigned nopen = 1; k < buf->size && nopen > 0; ++k)
			{
				wchar_t wch = buf_get_wch(buf, k);
				nopen += wch == L'<';
				nopen -= wch == L'>';
			}
		}
		
		while (k < buf->size && iswspace(buf_get_wch(buf, k)))
			++k;

		if (k < buf->size && buf_get_wch(buf, k) == L'(')
			wt = WT_FUNC;
	}
	
	for (size_t kw = 0; kw < ARRAY_SIZE(keywords); ++kw)
	{
		// no need for exact size match.
		// maybe in C++26 they'll add a 64-character long keyword, at
		// which point this will need to be increased.
		wchar_t cmp[64];
		buf_get_wstr(buf, cmp, *i, j - *i + 1);
		
		if (!wcscmp(keywords[kw], cmp))
		{
			wt = WT_KEYWORD;
			break;
		}
	}

	*out_lb = *i;
	*out_ub = j;

	switch (wt)
	{
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
