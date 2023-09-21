#ifndef MODE_C_H__
#define MODE_C_H__

#include "buf.h"

void mode_c_init(struct buf *b);
void mode_c_quit(struct buf *b);
void mode_c_keypress(struct buf *b, int k);

#endif
