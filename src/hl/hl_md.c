#include "hl/hl_md.h"

#include <wctype.h>

#include "conf.h"
#include "draw.h"

#define A_HEADING (A_MAGENTA | A_BGOF(CONF_A_NORM) | A_BRIGHT)
#define A_BLOCK (A_WHITE | A_BGOF(CONF_A_NORM) | A_DIM)
#define A_LINK (A_WHITE | A_BRED)
#define A_IMG (A_WHITE | A_BRED)
#define A_ULIST (A_MAGENTA | A_BGOF(CONF_A_NORM))
#define A_OLIST (A_MAGENTA | A_BGOF(CONF_A_NORM))

static int hl_codeblock(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_heading(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_link(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_img(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_ulist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_olist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);

int
hl_md_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'#') {
			if (!hl_heading(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'>') {
			if (!hl_block(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (i < len - 3 && !wcscmp(&src[i], L"```")) {
			if (!hl_codeblock(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'!') {
			if (!hl_img(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'*' || src[i] == L'-') {
			if (!hl_ulist(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (iswdigit(src[i])) {
			if (!hl_olist(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'[') {
			if (!hl_link(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_codeblock(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
             size_t *out_ub, uint16_t *a)
{
	return 1;
}

static int
hl_heading(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *oub_ub, uint16_t *out_a)
{
	return 1;
}

static int
hl_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint16_t *out_a)
{
	return 1;
}

static int
hl_link(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint16_t *out_a)
{
	return 1;
}

static int
hl_img(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
       size_t *out_ub, uint16_t *out_a)
{
	return 1;
}

static int
hl_ulist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint16_t *out_a)
{
	return 1;
}

static int
hl_olist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint16_t *out_a)
{
	return 1;
}
