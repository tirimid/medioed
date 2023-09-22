#ifndef UTIL_H__
#define UTIL_H__

#include <stdbool.h>
#include <stddef.h>

#include <unistd.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(min, n, max) MIN(MAX((min), (n)), (max))
#define ABS(n) ((n) < 0 ? -(n) : (n))
#define SIGN(n) ((n) / ABS(n))

struct arraylist {
	void **data;
	size_t *data_sizes;
	size_t size, cap;
};

struct string {
	char *data;
	size_t len, cap;
};

struct arraylist arraylist_create(void);
void arraylist_destroy(struct arraylist *al);
void arraylist_add(struct arraylist *al, void const *new, size_t size);
void arraylist_rm(struct arraylist *al, size_t ind);
void arraylist_swap(struct arraylist *al, size_t ind_a, size_t ind_b);
struct arraylist arraylist_copy(struct arraylist const *al);
ssize_t arraylist_find(struct arraylist const *al, void const *item, size_t size);

struct string string_create(void);
void string_destroy(struct string *s);
void string_push_ch(struct string *s, char ch);
void string_push_str(struct string *s, char const *str);
char *string_to_str(struct string const *s);

#endif
