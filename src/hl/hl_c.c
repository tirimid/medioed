#include "hl/hl_c.h"

#include <stdbool.h>
#include <string.h>
#include <wctype.h>

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

#define SPECIAL L"+-()[].<>{}!~*&/%=?:|;,"

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
static int hl_comment(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

static wchar_t const *keywords[] =
{
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
hl_c_find(struct buf const *buf, size_t off, size_t *out_lb, size_t *out_ub,
          uint8_t *out_fg, uint8_t *out_bg)
{
	for (size_t i = off; i < buf->size; ++i)
	{
		if (buf_get_wch(buf, i) == L'#')
		{
			if (!hl_preproc(buf, &i, out_lb, out_ub, out_fg, out_bg))
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
hl_preproc(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
           uint8_t *out_fg, uint8_t *out_bg)
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
hl_string(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
          uint8_t *out_fg, uint8_t *out_bg)
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
hl_char(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
        uint8_t *out_fg, uint8_t *out_bg)
{
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
hl_comment(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
           uint8_t *out_fg, uint8_t *out_bg)
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
hl_special(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
           uint8_t *out_fg, uint8_t *out_bg)
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
hl_word(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
        uint8_t *out_fg, uint8_t *out_bg)
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

		if (k < buf->size && buf_get_wch(buf, k) == L'(')
			wt = WT_FUNC;
	}
	
	for (size_t kw = 0; kw < ARRAY_SIZE(keywords); ++kw)
	{
		wchar_t cmp[64]; // read rationale for size 64 in C++ highlight.
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
