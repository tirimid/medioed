#ifndef MODE_MODE_LUA_H
#define MODE_MODE_LUA_H

#include <wchar.h>

#include "frame.h"

void mode_lua_init(struct frame *f);
void mode_lua_quit(void);
void mode_lua_update(void);
void mode_lua_keypress(wint_t k);

#endif
