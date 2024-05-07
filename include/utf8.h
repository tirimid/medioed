#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>
#include <stdint.h>

#define UTF8_BAD_CH 0xffffbadc

typedef uint32_t uchar32;

int uc32scmp(uchar32 const *lhs, uchar32 const *rhs);
int uc32sncmp(uchar32 const *lhs, uchar32 const *rhs, size_t n);
uchar32 utf8_decode_ch(uint8_t const *bytes);
uint8_t *utf8_encode_ch(uint8_t *out_bytes, uchar32 ch);
int utf8_decode_str(uchar32 *out_str, uint8_t const *bytes);
int utf8_encode_str(uint8_t *out_bytes, uchar32 const *str);

#endif
