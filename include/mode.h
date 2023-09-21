#ifndef MODE_H__
#define MODE_H__

#include "frame.h"

struct mode {
	char const *name;
	void (*init)(struct frame *), (*quit)(struct frame *);
	void (*keypress)(struct frame *, int);
};

struct mode const *mode_get(void);
void mode_set(char const *name, struct frame *f);
void mode_keyupdate(struct frame *f, int k);

#endif
