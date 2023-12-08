#ifndef MUTIL_H__
#define MUTIL_H__

#include "frame.h"

enum pairflags {
	PF_PAREN = 0x1,
	PF_BRACKET = 0x2,
	PF_BRACE = 0x4,
	PF_ANGLE = 0x8,
	PF_SQUOTE = 0x10,
	PF_DQUOTE = 0x20,
};

void mu_init(struct frame *f);
void mu_setbase(void);
void mu_setpairing(unsigned long flags);

#endif
