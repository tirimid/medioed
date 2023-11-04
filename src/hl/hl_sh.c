#include "hl/hl_sh.h"

#include "conf.h"
#include "draw.h"

#define A_KEYWORD (A_GREEN | A_BGOF(CONF_A_NORM))
#define A_BUILTIN (A_MAGENTA | A_BGOF(CONF_A_NORM) | A_BRIGHT)
#define A_COMMENT (A_RED | A_BGOF(CONF_A_NORM))
#define A_VAR (A_CYAN | A_BGOF(CONF_A_NORM))
#define A_STRING (A_YELLOW | A_BGOF(CONF_A_NORM))

int
hl_sh_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
           size_t *out_ub, uint16_t *out_a)
{
	return 1;
}
