#ifndef MODE_MODE_RS_H
#define MODE_MODE_RS_H

#include <wchar.h>

#include "frame.h"

void mode_rs_init(struct frame *f);
void mode_rs_quit(void);
void mode_rs_update(void);
void mode_rs_keypress(wint_t k);

#endif
