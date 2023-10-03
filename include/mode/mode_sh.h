#ifndef MODE_SH_H__
#define MODE_SH_H__

#include "frame.h"

void mode_sh_init(struct frame *f);
void mode_sh_quit(struct frame *f);
void mode_sh_keypress(struct frame *f, int k);

#endif
