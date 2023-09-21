#ifndef MODE_H__
#define MODE_H__

#include "buf.h"

struct mode {
	char const *name;
	void (*init)(struct buf *), (*quit)(struct buf *);
	void (*keypress)(struct buf *, int);
};

struct mode const *mode_get(void);
void mode_set(char const *name, struct buf *b);
void mode_keyupdate(struct buf *b, int k);

#endif
