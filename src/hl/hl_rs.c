#include "hl/hl_rs.h"

#include <string.h>
#include <wctype.h>

#include "conf.h"

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

#define SPECIAL L"!=%&*+,->./:;<@^|?#$(){}[]"

enum word_type
{
	WT_CONST,
	WT_TYPE,
	WT_FUNC,
	WT_KEYWORD,
	WT_BASIC,
};

static int hl_string(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_rstring(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_quote(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_ln_comment(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_blk_comment(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

static wchar_t const *keywords[] =
{
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
hl_rs_find(struct buf const *buf,
           size_t off,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	for (size_t i = off; i < buf->size; ++i)
	{
		if (buf_get_wch(buf, i) == L'"')
		{
			if (!hl_string(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (i + 1 < buf->size
		         && buf_get_wch(buf, i) == L'r'
		         && wcschr(L"\"#", buf_get_wch(buf, i + 1)))
		{
			if (!hl_rstring(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (buf_get_wch(buf, i) == L'\'')
		{
			if (!hl_quote(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (i + 1 < buf->size
		         && buf_get_wch(buf, i) == L'/'
		         && wcschr(L"/*", buf_get_wch(buf, i + 1)))
		{
			switch (buf_get_wch(buf, i + 1))
			{
			case L'/':
				if (!hl_ln_comment(buf, &i, out_lb, out_ub, out_fg, out_bg))
					return 0;
				break;
			case L'*':
				if (!hl_blk_comment(buf, &i, out_lb, out_ub, out_fg, out_bg))
					return 0;
				break;
			default:
				break;
			}
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
		else if (buf_get_wch(buf, j) == L'"')
			break;
	}
	
	*out_lb = *i;
	*out_ub = j + (j < buf->size);
	*out_fg = A_STRING_FG;
	*out_bg = A_STRING_BG;
	
	return 0;
}

static int
hl_rstring(struct buf const *buf,
           size_t *i,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < buf->size && buf_get_wch(buf, j) == L'#')
		++j;
	if (j >= buf->size || buf_get_wch(buf, j) != L'"')
		return 1;
	j += j < buf->size;
	
	unsigned nhash = (j - 1) - (*i + 1);

	wchar_t *search = malloc(sizeof(wchar_t) * (2 + nhash));
	search[0] = L'"';
	search[1 + nhash] = 0;
	for (unsigned i = 1; i < 1 + nhash; ++i)
		search[i] = L'#';
	
	while (j < buf->size)
	{
		// probably segfault if 64 or more hashes are used in a raw
		// string.
		// this is a non-issue probably not worth fully fixing.
		// TODO: make this not crash.
		wchar_t cmp[64];
		
		if (!wcscmp(buf_get_wstr(buf, cmp, j, 2 + nhash), search))
		{
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
hl_quote(struct buf const *buf,
         size_t *i,
         size_t *out_lb,
         size_t *out_ub,
         uint8_t *out_fg,
         uint8_t *out_bg)
{
	size_t j = *i + 1;
	if (j < buf->size && buf_get_wch(buf, j) == L'\\')
		j += 2;
	else
		++j;

	*out_lb = *i;
	if (j < buf->size && buf_get_wch(buf, j) == L'\'')
	{
		*out_ub = j + 1;
		*out_fg = A_STRING_FG;
		*out_bg = A_STRING_BG;
	}
	else
	{
		*out_ub = *i + 1;
		*out_fg = A_SPECIAL_FG;
		*out_bg = A_SPECIAL_BG;
	}

	return 0;
}

static int
hl_ln_comment(struct buf const *buf,
              size_t *i,
              size_t *out_lb,
              size_t *out_ub,
              uint8_t *out_fg,
              uint8_t *out_bg)
{
	size_t j = *i + 2;
	while (j < buf->size && buf_get_wch(buf, j) != L'\n')
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_COMMENT_FG;
	*out_bg = A_COMMENT_BG;
	
	return 0;
}

static int
hl_blk_comment(struct buf const *buf,
               size_t *i,
               size_t *out_lb,
               size_t *out_ub,
               uint8_t *out_fg,
               uint8_t *out_bg)
{
	unsigned nopen = 1;
	size_t j = *i + 2;
	while (j + 1 < buf->size)
	{
		wchar_t cmp_buf[3];
		buf_get_wstr(buf, cmp_buf, j, 3);
		
		if (!wcscmp(cmp_buf, L"*/"))
		{
			--nopen;
			++j;
		}
		else if (!wcscmp(cmp_buf, L"/*"))
		{
			++nopen;
			++j;
		}
		
		if (nopen == 0)
		{
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
	enum word_type wt = WT_BASIC;

	size_t j = *i;
	unsigned nunder = 0, nlower = 0, nupper = 0;
	while (j < buf->size)
	{
		wchar_t wch = buf_get_wch(buf, j);
		
		if (wch != L'_' && !iswalnum(wch))
			break;
		
		if (wch == L'_')
			++nunder;
		else if (iswlower(wch))
			++nlower;
		else if (iswupper(wch))
			++nupper;
		
		++j;
	}

	if (!nunder && nlower && nupper)
		wt = WT_TYPE;
	else if (!nlower && nupper)
		wt = WT_CONST;
	else
		wt = WT_BASIC;

	if (j - *i == 1 && buf_get_wch(buf, *i) == L'_')
		wt = WT_BASIC;

	if (wt == WT_BASIC)
	{
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
		
		wchar_t cmp_buf[4];
		if (k + 2 < buf->size
		    && !wcscmp(buf_get_wstr(buf, cmp_buf, k, 4), L"::<"))
		{
			k += 3;
			unsigned nopen = 1;
			while (k < buf->size && nopen > 0)
			{
				wchar_t wch = buf_get_wch(buf, k);
				nopen += wch == L'<';
				nopen -= wch == L'>';
				++k;
			}
		}
		
		if (k < buf->size && buf_get_wch(buf, k) == L'(')
			wt = WT_FUNC;
	}
	
	for (size_t kw = 0; kw < ARRAY_SIZE(keywords); ++kw)
	{
		size_t len = j - *i + 1;
		if (len > 64)
			break;
		
		wchar_t cmp[64];
		buf_get_wstr(buf, cmp, *i, len);
		
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
