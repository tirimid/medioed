#include "hl/hl_html.h"

#include <string.h>
#include <wctype.h>

#include "conf.h"
#include "draw.h"

#define A_TAG (A_MAGENTA | A_BG_OF(CONF_A_NORM) | A_BRIGHT)
#define A_ENT (A_MAGENTA | A_BG_OF(CONF_A_NORM))
#define A_COMMENT (A_RED | A_BG_OF(CONF_A_NORM))

static int hl_tag(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_ent(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);

int
hl_html_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
             size_t *out_ub, uint16_t *out_a)
{
	for (size_t i = off; i < len; ++i) {
		if (i + 3 < len && !wcsncmp(&src[i], L"<!--", 4)) {
			if (!hl_comment(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'<') {
			if (!hl_tag(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'&') {
			if (!hl_ent(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_tag(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
       size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	while (j < len && src[j] != L'>')
		++j;
	
	if (j == len)
		return 1;
	
	*out_lb = *i;
	*out_ub = j + 1;
	*out_a = A_TAG;
	
	return 0;
}

static int
hl_ent(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
       size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	while (j < len && iswalnum(src[j]))
		++j;
	
	if (j < len && src[j] == L';') {
		*out_lb = *i;
		*out_ub = j + 1;
		*out_a = A_ENT;
		
		return 0;
	}
	
	return 1;
}

static int
hl_comment(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
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
	*out_a = A_COMMENT;
	
	return 0;
}
