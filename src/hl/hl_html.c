#include "hl/hl_html.h"

#include <string.h>
#include <wctype.h>

#include "draw.h"
#include "hl/hldef.h"

#define A_TAG_FG HLD_A_ACCENT_1_FG
#define A_TAG_BG HLD_A_ACCENT_1_BG
#define A_ENT_FG HLD_A_ACCENT_2_FG
#define A_ENT_BG HLD_A_ACCENT_2_BG
#define A_COMMENT_FG HLD_A_COMMENT_FG
#define A_COMMENT_BG HLD_A_COMMENT_BG

static int hl_tag(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_ent(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

int
hl_html_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
             size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	for (size_t i = off; i < len; ++i) {
		if (i + 3 < len && !wcsncmp(&src[i], L"<!--", 4)) {
			if (!hl_comment(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'<') {
			if (!hl_tag(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (src[i] == L'&') {
			if (!hl_ent(src, len, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_tag(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
       size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < len && src[j] != L'>')
		++j;
	
	if (j == len)
		return 1;
	
	*out_lb = *i;
	*out_ub = j + 1;
	*out_fg = A_TAG_FG;
	*out_bg = A_TAG_BG;
	
	return 0;
}

static int
hl_ent(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
       size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < len && iswalnum(src[j]))
		++j;
	
	if (j < len && src[j] == L';') {
		*out_lb = *i;
		*out_ub = j + 1;
		*out_fg = A_ENT_FG;
		*out_bg = A_ENT_BG;
		
		return 0;
	}
	
	return 1;
}

static int
hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	size_t j = *i + 4;
	while (j < len && wcsncmp(&src[j], L"-->", 3))
		++j;
	
	if (j == len) {
		*i += 3;
		return 1;
	}
	
	*out_lb = *i;
	*out_ub = j + 3;
	*out_fg = A_COMMENT_FG;
	*out_bg = A_COMMENT_BG;
	
	return 0;
}
