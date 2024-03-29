#ifndef MODE_MODE_CS_H
#define MODE_MODE_CS_H

#include <wchar.h>

#include "frame.h"

void mode_cs_init(struct frame *f);
void mode_cs_quit(void);
void mode_cs_update(void);
void mode_cs_keypress(wint_t k);

#endif
