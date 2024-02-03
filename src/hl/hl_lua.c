#include "hl/hl_lua.h"

#define SPECIAL "+-*/%^#=~<>(){}[];:,."

static wchar_t const *keywords[] = {
	L"and",
	L"break",
	L"do",
	L"else",
	L"elseif",
	L"end",
	L"false",
	L"for",
	L"function",
	L"if",
	L"in",
	L"local",
	L"nil",
	L"not",
	L"or",
	L"repeat",
	L"return",
	L"then",
	L"true",
	L"until",
	L"while",
};

int
hl_lua_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
            size_t *out_ub, uint8_t *out_fg, uint8_t *out_bg)
{
	// TODO: implement lua highlighting.
	return 1;
}
