#include "hl/hl_c.h"

#include <stdbool.h>
#include <wctype.h>

#include "conf.h"
#include "draw.h"

#define A_PREPROC (A_CYAN | A_BGOF(CONF_A_NORM))
#define A_KEYWORD (A_GREEN | A_BGOF(CONF_A_NORM))
#define A_COMMENT (A_RED | A_BGOF(CONF_A_NORM))
#define A_STRING (A_YELLOW | A_BGOF(CONF_A_NORM))
#define A_FUNC (A_BRIGHT | A_MAGENTA | A_BGOF(CONF_A_NORM))
#define A_MACRO (A_CYAN | A_BGOF(CONF_A_NORM))
#define A_SPECIAL (A_BLUE | A_BGOF(CONF_A_NORM))
#define A_TRAILINGWS (A_FGOF(CONF_A_NORM) | A_BGREEN)

#define SPECIAL ""

static int hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_trailingws(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);

int
hl_c_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
          size_t *out_ub, uint16_t *out_a)
{
	for (size_t i = off; i < len; ++i) {
		if (src[i] == L'"') {
			if (!hl_string(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'\'') {
			if (!hl_char(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (iswspace(src[i])) {
			if (!hl_trailingws(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		}
	}
	
	return 1;
}

static int
hl_string(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
          size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i;
	while (j < len) {
		++j;
		if (src[j] == L'\\') {
			++j;
			continue;
		} else if (src[j] == L'"' || src[j] == L'\n')
			break;
	}
	
	*out_lb = *i;
	*out_ub = j + 1;
	*out_a = A_STRING;
	
	return 0;
}

static int
hl_char(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
        size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	if (j < len && src[j] == L'\\')
		j += 2;
	else
		++j;

	if (j < len && src[j] == L'\'') {
		*out_lb = *i;
		*out_ub = j + 1;
		*out_a = A_STRING;
		
		return 0;
	}
	
	return 1;
}

static int
hl_special(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
{
	return 1;
}

static int
hl_trailingws(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
              size_t *out_ub, uint16_t *out_a)
{
	size_t j = *i + 1;
	while (j < len && iswspace(src[j]) && src[j] != L'\n')
		++j;
	
	if (j < len && src[j] == L'\n') {
		*out_lb = *i;
		*out_ub = j;
		*out_a = A_TRAILINGWS;
		
		return 0;
	} else
		*i = j - 1;

	return 1;
}
