#ifndef MODE_MODE_C_H__
#define MODE_MODE_C_H__

#include <wchar.h>

#include "frame.h"

void mode_c_init(struct frame *f);
void mode_c_quit(void);
void mode_c_update(void);
void mode_c_keypress(wint_t k);

#endif
