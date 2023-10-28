#ifndef HL_S_H__
#define HL_S_H__

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

int hl_s_find(wchar_t const *src, size_t len, size_t off, size_t *out_lb,
              size_t *out_ub, uint16_t *out_a);

#endif
