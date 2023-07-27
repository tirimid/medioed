#ifndef UTIL_H__
#define UTIL_H__

#include <stdbool.h>
#include <stddef.h>

struct arraylist {
	void **data;
	size_t *data_sizes;
	size_t size, cap;
};

struct arraylist arraylist_create(void);
void arraylist_destroy(struct arraylist *al);
void arraylist_add(struct arraylist *al, void const *new, size_t size);
void arraylist_rm(struct arraylist *al, size_t ind);
struct arraylist arraylist_copy(struct arraylist const *al);
bool arraylist_contains(struct arraylist const *al, void const *item,
                        size_t size);

#endif
