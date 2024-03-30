#ifndef LABEL_H
#define LABEL_H

#include <wchar.h>

struct label_bounds {
	unsigned pr, pc;
	unsigned sr, sc;
};

struct label_bounds label_rebound(struct label_bounds const *bounds, unsigned anchor_l, unsigned anchor_r, unsigned anchor_t);
int label_show(wchar_t const *name, wchar_t const *msg, struct label_bounds const *bounds);

#endif
