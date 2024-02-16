#ifndef MODE_MODE_S_H
#define MODE_MODE_S_H

#include <wchar.h>

#include "frame.h"

void mode_s_init(struct frame *f);
void mode_s_quit(void);
void mode_s_update(void);
void mode_s_keypress(wint_t k);

#endif
