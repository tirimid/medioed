#ifndef MODE_MODE_CLIKE_H
#define MODE_MODE_CLIKE_H

#include <wchar.h>

#include "frame.h"

void mode_clike_init(struct frame *f);
void mode_clike_quit(void);
void mode_clike_update(void);
void mode_clike_keypress(wint_t k);

#endif

