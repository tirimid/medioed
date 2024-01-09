#include "minicalc.h"

#include <stdio.h>
#include <wchar.h>

enum tokentype {
	TT_IDENT = 0,
};

struct token {
	unsigned char type;
};

int
minicalc_eval(wchar_t *out_val, size_t outsize, wchar_t const *src)
{
	swprintf(out_val, outsize, L"evaluation not implemented");
	return 0;
}
