#include "hl/hl_s.h"

#include <string.h>
#include <wctype.h>

#include "conf.h"

// implemented only for x86_64 since that's the only assembly I write.
// instruction mnemonics are not highlighted since there are way too many of
// them to do a basic keyword highlighting system.

#define A_PREPROC_FG CONF_A_ACCENT_3_FG
#define A_PREPROC_BG CONF_A_ACCENT_3_BG
#define A_REG_FG CONF_A_ACCENT_1_FG
#define A_REG_BG CONF_A_ACCENT_1_BG
#define A_COMMENT_FG CONF_A_COMMENT_FG
#define A_COMMENT_BG CONF_A_COMMENT_BG
#define A_STRING_FG CONF_A_STRING_FG
#define A_STRING_BG CONF_A_STRING_BG
#define A_MACRO_FG CONF_A_ACCENT_2_FG
#define A_MACRO_BG CONF_A_ACCENT_2_BG
#define A_SPECIAL_FG CONF_A_SPECIAL_FG
#define A_SPECIAL_BG CONF_A_SPECIAL_BG

// same as C special chars but missing `.` for preprocessor directives.
#define SPECIAL L"+-()[]<>{}!~*&/%=?:|;,^"

enum word_type
{
	WT_MACRO,
	WT_REG,
	WT_BASIC,
};

static int hl_preproc(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_comment(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_string(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_char(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

static wchar_t const *regs[] =
{
	L"rax",
	L"eax",
	L"ax",
	L"ah",
	L"al",
	L"rbx",
	L"ebx",
	L"bx",
	L"bh",
	L"bl",
	L"rcx",
	L"ecx",
	L"cx",
	L"ch",
	L"cl",
	L"rdx",
	L"edx",
	L"dx",
	L"dh",
	L"dl",
	L"rsi",
	L"esi",
	L"si",
	L"sil",
	L"rdi",
	L"edi",
	L"di",
	L"dil",
	L"rsp",
	L"esp",
	L"sp",
	L"spl",
	L"rbp",
	L"ebp",
	L"bp",
	L"bpl",
	L"r8",
	L"r8d",
	L"r8w",
	L"r8b",
	L"r9",
	L"r9d",
	L"r9w",
	L"r9b",
	L"r10",
	L"r10d",
	L"r10w",
	L"r10b",
	L"r11",
	L"r11d",
	L"r11w",
	L"r11b",
	L"r12",
	L"r12d",
	L"r12w",
	L"r12b",
	L"r13",
	L"r13d",
	L"r13w",
	L"r13b",
	L"r14",
	L"r14d",
	L"r14w",
	L"r14b",
	L"r15",
	L"r15d",
	L"r15w",
	L"r15b",
	L"rip",
	L"eip",
	L"ip",
	L"cs",
	L"ds",
	L"es",
	L"fs",
	L"gs",
	L"ss",
	L"cr0",
	L"cr1",
	L"cr2",
	L"cr3",
	L"cr4",
	L"cr5",
	L"cr6",
	L"cr7",
	L"cr8",
	L"cr9",
	L"cr10",
	L"cr11",
	L"cr12",
	L"cr13",
	L"cr14",
	L"cr15",
	L"dr0",
	L"dr1",
	L"dr2",
	L"dr3",
	L"dr4",
	L"dr5",
	L"dr6",
	L"dr7",
};

int
hl_s_find(struct buf const *buf,
          size_t off,
          size_t *out_lb,
          size_t *out_ub,
          uint8_t *out_fg,
          uint8_t *out_bg)
{
	for (size_t i = off; i < buf->size; ++i)
	{
		if (buf_get_wch(buf, i) == L'.')
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
	size_t j = *i + 1;
	while (j < buf->size)
	{
		if (buf_get_wch(buf, j) != L'_'
		    && !iswalnum(buf_get_wch(buf, j)))
		{
			break;
		}
		++j;
	}
	
	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_PREPROC_FG;
	*out_bg = A_PREPROC_BG;
	
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
		while (j < buf->size)
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
	size_t j = *i + 1;
	if (j < buf->size && buf_get_wch(buf, j) == L'\\')
		j += 2;
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
	
	for (size_t reg = 0; reg < ARRAY_SIZE(regs); ++reg)
	{
		size_t len = j - *i + 1;
		if (len > 64)
			break;
		
		wchar_t cmp[64];
		buf_get_wstr(buf, cmp, *i, len);
		
		if (!wcscmp(regs[reg], cmp))
		{
			wt = WT_REG;
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
	case WT_REG:
		*out_fg = A_REG_FG;
		*out_bg = A_REG_BG;
		break;
	case WT_BASIC:
		*i = j - 1;
		return 1;
	}
	
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
