#include "hl/hl_md.h"

#include <wctype.h>

#include "draw.h"
#include "hl/hldef.h"

#define A_CODE_BLOCK_FG HLD_A_ACCENT_3_FG
#define A_CODE_BLOCK_BG HLD_A_ACCENT_3_BG
#define A_HEADING_FG HLD_A_ACCENT_2_FG
#define A_HEADING_BG HLD_A_ACCENT_2_BG
#define A_BLOCK_FG HLD_A_ACCENT_4_FG
#define A_BLOCK_BG HLD_A_ACCENT_4_BG
#define A_ULIST_FG HLD_A_ACCENT_1_FG
#define A_ULIST_BG HLD_A_ACCENT_1_BG
#define A_OLIST_FG HLD_A_ACCENT_1_FG
#define A_OLIST_BG HLD_A_ACCENT_1_BG

static int hl_code_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_heading(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_ulist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_olist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static size_t first_ln_ch(wchar_t const *src, size_t len, size_t pos);
static size_t para_end(wchar_t const *src, size_t len, size_t pos);

int
hl_md_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'#') {
			if (!hl_heading(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'>') {
			if (!hl_block(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (i + 2 < len && !wcsncmp(&src[i], L"```", 3)) {
			if (!hl_code_block(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'*' || src[i] == L'-') {
			if (!hl_ulist(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (iswdigit(src[i])) {
			if (!hl_olist(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'\\') {
			++i;
			continue;
		}
	}
	
	return 1;
}

static int
hl_code_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
              size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(src, len, *i) != *i)
		return 1;
	
	for (size_t j = *i + 3; j + 2 < len; ++j) {
		if (src[j] == L'\\') {
			++j;
			continue;
		} else if (!wcsncmp(&src[j], L"```", 3)) {
			if (first_ln_ch(src, len, j) != j)
				continue;
			
			for (size_t k = j + 3; k < len && src[k] != L'\n'; ++k) {
				if (!iswspace(src[k]))
					goto skip;
			}
			
			*out_lb = *i;
			*out_ub = j + 3;
			*out_fg = A_CODE_BLOCK_FG;
			*out_bg = A_CODE_BLOCK_BG;
			
			return 0;
		}
	skip:;
	}
	
	return 1;
}

static int
hl_heading(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(src, len, *i) != *i)
		return 1;

	size_t j = *i + 1;
	while (j < len && src[j] == L'#')
		++j;
	if (j >= len || !iswspace(src[j]) || src[j] == L'\n')
		return 1;
	
	while (j < len && src[j] != L'\n')
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_HEADING_FG;
	*out_bg = A_HEADING_BG;
	
	return 0;
}

static int
hl_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(src, len, *i) != *i)
		return 1;
	
	size_t j = *i + 1;
	while (j < len && iswspace(src[j]) && src[j] != L'\n')
		++j;
	if (j >= len || iswspace(src[j]))
		return 1;
	
	*out_lb = *i;
	*out_ub = para_end(src, len, j);
	*out_fg = A_BLOCK_FG;
	*out_bg = A_BLOCK_BG;
	
	return 0;
}

static int
hl_ulist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(src, len, *i) != *i)
		return 1;
	
	size_t j = *i + 1;
	while (j < len && src[j] == src[*i])
		++j;
	if (j >= len || !iswspace(src[j]) || src[j] == L'\n')
		return 1;
	
	*out_lb = *i;
	*out_ub = para_end(src, len, j);
	*out_fg = A_ULIST_FG;
	*out_bg = A_ULIST_BG;
	
	return 0;
}

static int
hl_olist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(src, len, *i) != *i)
		return 1;

	size_t j = *i + 1;
	while (j < len && iswdigit(src[j]))
		++j;
	if (j + 1 >= len
	    || src[j] != L'.'
	    || !iswspace(src[j + 1])
	    || src[j + 1] == L'\n') {
		return 1;
	}

	*out_lb = *i;
	*out_ub = para_end(src, len, j + 1);
	*out_fg = A_OLIST_FG;
	*out_bg = A_OLIST_BG;
	
	return 0;
}

static size_t
first_ln_ch(wchar_t const *src, size_t len, size_t pos)
{
	size_t ln = pos;
	while (ln > 0 && src[ln] != L'\n')
		--ln;
	
	size_t firstch = ln;
	while (firstch < len && iswspace(src[firstch]))
		++firstch;
	
	return firstch;
}

static size_t
para_end(wchar_t const *src, size_t len, size_t pos)
{
	size_t i = pos;
	
	for (;;) {
		while (i < len && src[i] != L'\n')
			++i;
		
		i += i < len;
		while (i < len && iswspace(src[i]) && src[i] != L'\n')
			++i;
	
		if (i >= len || src[i] == L'\n')
			break;
	}

	return i;
}
