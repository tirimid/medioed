#include "hl/hl_md.h"

#include <wctype.h>

#include "conf.h"
#include "draw.h"

#define A_CODEBLOCK (A_YELLOW | A_BGOF(CONF_A_NORM))
#define A_HEADING (A_MAGENTA | A_BGOF(CONF_A_NORM) | A_BRIGHT)
#define A_BLOCK (A_WHITE | A_BGOF(CONF_A_NORM) | A_DIM)
#define A_ULIST (A_MAGENTA | A_BGOF(CONF_A_NORM))
#define A_OLIST (A_MAGENTA | A_BGOF(CONF_A_NORM))

static int hl_codeblock(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_heading(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_ulist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static int hl_olist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb, size_t *out_ub, uint16_t *out_a);
static size_t firstlnch(wchar_t const *src, size_t len, size_t pos);
static size_t paraend(wchar_t const *src, size_t len, size_t pos);

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
		} else if (i + 2 < len && !wcsncmp(&src[i], L"```", 3)) {
			if (!hl_codeblock(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'*' || src[i] == L'-') {
			if (!hl_ulist(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (iswdigit(src[i])) {
			if (!hl_olist(src, len, &i, out_lb, out_ub, out_a))
				return 0;
		} else if (src[i] == L'\\') {
			++i;
			continue;
		}
	}
	
	return 1;
}

static int
hl_codeblock(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
             size_t *out_ub, uint16_t *out_a)
{
	if (firstlnch(src, len, *i) != *i)
		return 1;
	
	for (size_t j = *i + 3; j + 2 < len; ++j) {
		if (src[j] == L'\\') {
			++j;
			continue;
		} else if (!wcsncmp(&src[j], L"```", 3)) {
			if (firstlnch(src, len, j) != j)
				continue;
			
			for (size_t k = j + 3; k < len && src[k] != L'\n'; ++k) {
				if (!iswspace(src[k]))
					goto skip;
			}
			
			*out_lb = *i;
			*out_ub = j + 3;
			*out_a = A_CODEBLOCK;
			
			return 0;
		}
	skip:;
	}
	
	return 1;
}

static int
hl_heading(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
{
	if (firstlnch(src, len, *i) != *i)
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
	*out_a = A_HEADING;
	
	return 0;
}

static int
hl_block(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint16_t *out_a)
{
	if (firstlnch(src, len, *i) != *i)
		return 1;
	
	size_t j = *i + 1;
	while (j < len && iswspace(src[j]) && src[j] != L'\n')
		++j;
	if (j >= len || iswspace(src[j]))
		return 1;
	
	*out_lb = *i;
	*out_ub = paraend(src, len, j);
	*out_a = A_BLOCK;
	
	return 0;
}

static int
hl_ulist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint16_t *out_a)
{
	if (firstlnch(src, len, *i) != *i)
		return 1;
	
	size_t j = *i + 1;
	while (j < len && src[j] == src[*i])
		++j;
	if (j >= len || !iswspace(src[j]) || src[j] == L'\n')
		return 1;
	
	*out_lb = *i;
	*out_ub = paraend(src, len, j);
	*out_a = A_ULIST;
	
	return 0;
}

static int
hl_olist(wchar_t const *src, size_t len, size_t *i, size_t *out_lb,
         size_t *out_ub, uint16_t *out_a)
{
	if (firstlnch(src, len, *i) != *i)
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
	*out_ub = paraend(src, len, j + 1);
	*out_a = A_OLIST;
	
	return 0;
}

static size_t
firstlnch(wchar_t const *src, size_t len, size_t pos)
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
paraend(wchar_t const *src, size_t len, size_t pos)
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
