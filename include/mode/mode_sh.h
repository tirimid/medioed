#ifndef MODE_MODE_SH_H__
#define MODE_MODE_SH_H__

#include <wchar.h>

#include "frame.h"

void mode_sh_init(struct frame *f);
void mode_sh_quit(void);
void mode_sh_update(void);
void mode_sh_keypress(wint_t k);

#endif
