#ifndef MODE_C_H__
#define MODE_C_H__

#include <wchar.h>

#include "frame.h"

void mode_c_init(struct frame *f);
void mode_c_quit(struct frame *f);
void mode_c_keypress(struct frame *f, wint_t k);

#endif
