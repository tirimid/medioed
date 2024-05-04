#ifndef MODE_H
#define MODE_H

#include <wchar.h>

#include "frame.h"

struct mode
{
	char const *name;
	void (*init)(struct frame *), (*quit)(void);
	void (*update)(void);
	void (*keypress)(wint_t);
};

struct mode const *mode_get(void);
void mode_set(char const *name, struct frame *f);
void mode_update(void);
void mode_key_update(wint_t k);

#endif
