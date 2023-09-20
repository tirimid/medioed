#ifndef MODE_H__
#define MODE_H__

#include "buf.h"

struct mode {
	char const *name;
	void (*init)(struct buf *), (*quit)(struct buf *);
	void (*keypress)(struct buf *, int);
};

void mode_sys_init(void);
void mode_sys_quit(void);
void mode_add(struct mode const *m);
void mode_rm(char const *name);
void mode_set(char const *name);
void mode_unset(void);

#endif
