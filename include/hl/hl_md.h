#ifndef HL_HL_MD_H
#define HL_HL_MD_H

#include <stddef.h>
#include <stdint.h>

#include "buf.h"

int hl_md_find(struct buf const *buf,
               size_t off,
               size_t *out_lb,
               size_t *out_ub,
               uint8_t *out_fg,
               uint8_t *out_bg);

#endif
