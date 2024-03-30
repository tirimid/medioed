#include "hl/hl_md.h"

#include <wctype.h>

#include "conf.h"

#define A_CODE_BLOCK_FG CONF_A_ACCENT_3_FG
#define A_CODE_BLOCK_BG CONF_A_ACCENT_3_BG
#define A_HEADING_FG CONF_A_ACCENT_2_FG
#define A_HEADING_BG CONF_A_ACCENT_2_BG
#define A_BLOCK_FG CONF_A_ACCENT_4_FG
#define A_BLOCK_BG CONF_A_ACCENT_4_BG
#define A_ULIST_FG CONF_A_ACCENT_1_FG
#define A_ULIST_BG CONF_A_ACCENT_1_BG
#define A_OLIST_FG CONF_A_ACCENT_1_FG
#define A_OLIST_BG CONF_A_ACCENT_1_BG

static int hl_code_block(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_heading(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_block(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_ulist(struct buf const *buf_t, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_olist(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static size_t first_ln_ch(struct buf const *buf, size_t pos);
static size_t para_end(struct buf const *buf, size_t pos);

int
hl_md_find(struct buf const *buf, size_t off, size_t *out_lb, size_t *out_ub,
           uint8_t *out_fg, uint8_t *out_bg)
{
	for (size_t i = off; i < buf->size; ++i) {
		wchar_t cmp_buf[4];
		
		if (buf_get_wch(buf, i) == L'#') {
			if (!hl_heading(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (buf_get_wch(buf, i) == L'>') {
			if (!hl_block(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (i + 2 < buf->size
		           && !wcscmp(buf_get_wstr(buf, cmp_buf, i, 3), L"```")) {
			if (!hl_code_block(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (wcschr(L"*-", buf_get_wch(buf, i))) {
			if (!hl_ulist(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (iswdigit(buf_get_wch(buf, i))) {
			if (!hl_olist(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		} else if (buf_get_wch(buf, i) == L'\\') {
			++i;
			continue;
		}
	}
	
	return 1;
}

static int
hl_code_block(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
              uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(buf, *i) != *i)
		return 1;
	
	for (size_t j = *i + 3; j + 2 < buf->size; ++j) {
		wchar_t cmp_buf[4];
		
		if (buf_get_wch(buf, j) == L'\\') {
			++j;
			continue;
		} else if (!wcscmp(buf_get_wstr(buf, cmp_buf, j, 3), L"```")) {
			if (first_ln_ch(buf, j) != j)
				continue;
			
			size_t k = j + 3;
			while (k < buf->size && buf_get_wch(buf, k) != L'\n') {
				if (!iswspace(buf_get_wch(buf, k)))
					goto skip;
				++k;
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
hl_heading(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
           uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(buf, *i) != *i)
		return 1;
	
	size_t j = *i + 1;
	while (j < buf->size && buf_get_wch(buf, j) == L'#')
		++j;
	if (j >= buf->size
	    || !iswspace(buf_get_wch(buf, j))
	    || buf_get_wch(buf, j) == L'\n') {
		return 1;
	}
	
	while (j < buf->size && buf_get_wch(buf, j) != L'\n')
		++j;

	*out_lb = *i;
	*out_ub = j;
	*out_fg = A_HEADING_FG;
	*out_bg = A_HEADING_BG;
	
	return 0;
}

static int
hl_block(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
         uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(buf, *i) != *i)
		return 1;
	
	size_t j = *i + 1;
	while (j < buf->size
	       && iswspace(buf_get_wch(buf, j))
	       && buf_get_wch(buf, j) != L'\n') {
		++j;
	}
	if (j >= buf->size || iswspace(buf_get_wch(buf, j)))
		return 1;
	
	*out_lb = *i;
	*out_ub = para_end(buf, j);
	*out_fg = A_BLOCK_FG;
	*out_bg = A_BLOCK_BG;
	
	return 0;
}

static int
hl_ulist(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
         uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(buf, *i) != *i)
		return 1;
	
	size_t j = *i + 1;
	while (j < buf->size && buf_get_wch(buf, j) == buf_get_wch(buf, *i))
		++j;
	if (j >= buf->size
	    || !iswspace(buf_get_wch(buf, j))
	    || buf_get_wch(buf, j) == L'\n') {
		return 1;
	}
	
	*out_lb = *i;
	*out_ub = para_end(buf, j);
	*out_fg = A_ULIST_FG;
	*out_bg = A_ULIST_BG;
	
	return 0;
}

static int
hl_olist(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub,
         uint8_t *out_fg, uint8_t *out_bg)
{
	if (first_ln_ch(buf, *i) != *i)
		return 1;

	size_t j = *i + 1;
	while (j < buf->size && iswdigit(buf_get_wch(buf, j)))
		++j;
	if (j + 1 >= buf->size
	    || buf_get_wch(buf, j) != L'.'
	    || !iswspace(buf_get_wch(buf, j + 1))
	    || buf_get_wch(buf, j + 1) == L'\n') {
		return 1;
	}
	
	*out_lb = *i;
	*out_ub = para_end(buf, j + 1);
	*out_fg = A_OLIST_FG;
	*out_bg = A_OLIST_BG;
	
	return 0;
}

static size_t
first_ln_ch(struct buf const *buf, size_t pos)
{
	size_t ln = pos;
	while (ln > 0 && buf_get_wch(buf, ln) != L'\n')
		--ln;
	
	size_t first_ch = ln;
	while (first_ch < buf->size && iswspace(buf_get_wch(buf, first_ch)))
		++first_ch;
	
	return first_ch;
}

static size_t
para_end(struct buf const *buf, size_t pos)
{
	size_t i = pos;
	
	for (;;) {
		while (i < buf->size && buf_get_wch(buf, i) != L'\n')
			++i;
		
		i += i < buf->size;
		while (i < buf->size
		       && iswspace(buf_get_wch(buf, i))
		       && buf_get_wch(buf, i) != L'\n') {
			++i;
		}
	
		if (i >= buf->size || buf_get_wch(buf, i) == L'\n')
			break;
	}

	return i;
}
