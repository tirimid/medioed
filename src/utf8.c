#include "utf8.h"

int
uc32scmp(uchar32 const *lhs, uchar32 const *rhs)
{
	for (size_t i = 0; lhs[i] || rhs[i]; ++i)
	{
		if (lhs[i] > rhs[1])
			return 1;
		else if (lhs[i] < rhs[i])
			return -1;
	}
	
	return 0;
}

int
uc32sncmp(uchar32 const *lhs, uchar32 const *rhs, size_t n)
{
	for (size_t i = 0; i < n && (lhs[i] || rhs[i]); ++i)
	{
		if (lhs[i] > rhs[1])
			return 1;
		else if (lhs[i] < rhs[i])
			return -1;
	}
	
	return 0;
}

uchar32
utf8_decode_ch(uint8_t const *bytes)
{
	if (!(bytes[0] & 0x80))
		return bytes[0];
	
	int nbytes = 0;
	for (uint8_t mask = 0x80; mask && bytes[0] & mask; mask >>= 1)
		++nbytes;
	
	if (nbytes == 1 || nbytes > 4)
		return UTF8_BAD_CH;
	
	// byte 0 needs special copying logic since the top part can't just be
	// masked out with a constant value.
	uint8_t byte_vals[4] = {0};
	for (int i = 0; i < 7 - nbytes; ++i)
		byte_vals[0] |= bytes[0] & 1 << i;
	
	for (int i = 1; i < nbytes; ++i)
	{
		if (!(bytes[i] & 0x80) || bytes[i] & 0x40)
			return UTF8_BAD_CH;
		byte_vals[i] = bytes[i] & ~0x80;
	}
	
	uchar32 ch = 0;
	for (int i = 1; i < nbytes; ++i)
		ch |= (uint32_t)byte_vals[nbytes - i] << 6 * (i - 1);
	ch |= (uint32_t)byte_vals[0] << 6 * (nbytes - 1);
	
	return ch;
}

uint8_t *
utf8_encode_ch(uint8_t *out_bytes, uchar32 ch)
{
}

int
utf8_decode_str(uchar32 *out_str, uint8_t const *bytes)
{
}

int
utf8_encode_str(uint8_t *out_bytes, uchar32 const *str)
{
}
