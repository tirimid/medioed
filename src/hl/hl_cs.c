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

#define SPECIAL L"+-()[].<>{}!~*&/%=?:|;,^"

enum word_type
{
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
hl_cs_find(struct buf const *buf,
           size_t off,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
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
hl_preproc(struct buf const *buf,
           size_t *i,
           size_t *out_lb,
           size_t *out_ub,
           uint8_t *out_fg,
           uint8_t *out_bg)
{
	size_t j = *i;
	while (j < buf->size && buf_get_wch(buf, j) != L'\n')
		++j;
	
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
	// TODO: implement.
	// maybe a separate function should be used for raw string highlight.
	return 1;
}

static int
hl_char(struct buf const *buf,
        size_t *i,
        size_t *out_lb,
        size_t *out_ub,
        uint8_t *out_fg,
        uint8_t *out_bg)
{
	size_t j = *i + 1;
	if (j < buf->size && buf_get_wch(buf, j) == L'\\')
	{
		++j;
		if (j < buf->size && wcschr(L"xu", buf_get_wch(buf, j)))
		{
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
		while (j + 1 < buf->size)
		{
			wchar_t cmp[3];
			if (!wcscmp(buf_get_wstr(buf, cmp, j, 3), L"*/"))
				break;
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
	enum word_type wt = WT_BASIC;
	bool ident_pfx = *i > 0 && buf_get_wch(buf, *i - 1) == L'@';
	
	size_t j = *i;
	while (j < buf->size)
	{
		if (buf_get_wch(buf, j) != L'_'
		    && !iswalnum(buf_get_wch(buf, j)))
		{
			break;
		}
		++j;
	}
	
	// check for (generic) function.
	// similarly to C++ highlight mode template checking, this is very
	// stupid and doesn't account for embedded comments.
	// however, this defect arguably doesn't even slightly matter.
	
	size_t k = j;
	while (k < buf->size && iswspace(buf_get_wch(buf, k)))
		++k;
	
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
	
	if (!ident_pfx)
	{
		for (size_t kw = 0; kw < ARRAY_SIZE(keywords); ++kw)
		{
			// rationale for size 64 in C++ highlight.
			wchar_t cmp[64];
			buf_get_wstr(buf, cmp, *i, j - *i + 1);
			
			if (!wcscmp(keywords[kw], cmp))
			{
				wt = WT_KEYWORD;
				break;
			}
		}
	}
	
	*out_lb = *i;
	*out_ub = j;
	
	switch (wt)
	{
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
