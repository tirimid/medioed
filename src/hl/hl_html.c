#include "hl/hl_html.h"

#include <string.h>
#include <wctype.h>

#include "conf.h"

#define A_TAG_FG CONF_A_ACCENT_1_FG
#define A_TAG_BG CONF_A_ACCENT_1_BG
#define A_ENT_FG CONF_A_ACCENT_2_FG
#define A_ENT_BG CONF_A_ACCENT_2_BG
#define A_COMMENT_FG CONF_A_COMMENT_FG
#define A_COMMENT_BG CONF_A_COMMENT_BG

static int hl_tag(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_ent(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);
static int hl_comment(struct buf const *buf, size_t *i, size_t *out_lb, size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg);

int
hl_html_find(struct buf const *buf,
             size_t off,
             size_t *out_lb,
             size_t *out_ub,
             uint8_t *out_fg,
             uint8_t *out_bg)
{
	for (size_t i = off; i < buf->size; ++i)
	{
		wchar_t cmp[5];
		
		if (i + 3 < buf->size
		    && !wcscmp(buf_get_wstr(buf, cmp, i, 5), L"<!--"))
		{
			if (!hl_comment(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (buf_get_wch(buf, i) == L'<')
		{
			if (!hl_tag(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
		else if (buf_get_wch(buf, i) == L'&')
		{
			if (!hl_ent(buf, &i, out_lb, out_ub, out_fg, out_bg))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_tag(struct buf const *buf,
       size_t *i,
       size_t *out_lb,
       size_t *out_ub,
       uint8_t *out_fg,
       uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < buf->size && buf_get_wch(buf, j) != L'>')
		++j;
	
	if (j >= buf->size)
		return 1;
	
	*out_lb = *i;
	*out_ub = j + 1;
	*out_fg = A_TAG_FG;
	*out_bg = A_TAG_BG;
	
	return 0;
}

static int
hl_ent(struct buf const *buf,
       size_t *i,
       size_t *out_lb,
       size_t *out_ub,
       uint8_t *out_fg,
       uint8_t *out_bg)
{
	size_t j = *i + 1;
	while (j < buf->size && iswalnum(buf_get_wch(buf, j)))
		++j;
	
	if (j < buf->size && buf_get_wch(buf, j) == L';')
	{
		*out_lb = *i;
		*out_ub = j + 1;
		*out_fg = A_ENT_FG;
		*out_bg = A_ENT_BG;
		
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
	size_t j = *i + 4;
	wchar_t cmp[4];
	while (j < buf->size && wcscmp(buf_get_wstr(buf, cmp, j, 4), L"-->"))
		++j;
	
	if (j == buf->size)
	{
		*i += 3;
		return 1;
	}
	
	*out_lb = *i;
	*out_ub = j + 3;
	*out_fg = A_COMMENT_FG;
	*out_bg = A_COMMENT_BG;
	
	return 0;
}
