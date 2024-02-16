#ifndef MODE_MODE_MD_H
#define MODE_MODE_MD_H

#include <wchar.h>

#include "frame.h"

void mode_md_init(struct frame *f);
void mode_md_quit(void);
void mode_md_update(void);
void mode_md_keypress(wint_t k);

#endif
