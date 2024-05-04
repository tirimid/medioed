#ifndef MODE_MODE_NULL_H
#define MODE_MODE_NULL_H

#include <wchar.h>

#include "frame.h"

void mode_null_init(struct frame *f);
void mode_null_quit(void);
void mode_null_update(void);
void mode_null_keypress(wint_t k);

#endif

