#ifndef MODE_SH_H__
#define MODE_SH_H__

#include <wchar.h>

#include "frame.h"

void mode_sh_init(struct frame *f);
void mode_sh_quit(struct frame *f);
void mode_sh_keypress(struct frame *f, wint_t k);

#endif
