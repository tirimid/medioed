#ifndef MODE_MODE_CC_H__
#define MODE_MODE_CC_H__

#include <wchar.h>

#include "frame.h"

void mode_cc_init(struct frame *f);
void mode_cc_quit(void);
void mode_cc_update(void);
void mode_cc_keypress(wint_t k);

#endif
