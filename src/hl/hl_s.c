#include "hl/hl_s.h"

#include <string.h>
#include <wctype.h>

// implemented only for x86_64 since that's the only assembly I write.
// instruction mnemonics are not highlighted since there are way too many of
// them.

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
#define SPECIAL L"+-()[]<>{}!~*&/%=?:|;,"

enum word_type {
	WT_MACRO,
	WT_REG,
	WT_BASIC,
};

static int hl_preproc(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

static wchar_t const *regs[] = {
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
hl_s_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
          size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'.') {
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
	return 1;
}

static int
hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	return 1;
}

static int
hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
          size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	return 1;
}

static int
hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	return 1;
}

static int
hl_word(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	return 1;
}

static int
hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	return 1;
}
