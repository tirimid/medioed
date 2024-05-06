#ifndef HL_HL_CC_H
#define HL_HL_CC_H

#include <stddef.h>
#include <stdint.h>

#include "buf.h"

int hl_cc_find(struct buf const *buf,
               size_t off,
               size_t *out_lb,
               size_t *out_ub,
               uint8_t *out_fg,
               uint8_t *out_bg);

#endif
