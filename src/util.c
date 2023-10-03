#include "util.h"

#include <stdlib.h>
#include <string.h>

struct arraylist
arraylist_create(void)
{
	return (struct arraylist){
		.data = malloc(sizeof(void *)),
		.data_sizes = malloc(sizeof(size_t)),
		.size = 0,
		.cap = 1,
	};
}

void
arraylist_destroy(struct arraylist *al)
{
	for (size_t i = 0; i < al->size; ++i)
		free(al->data[i]);

	free(al->data);
	free(al->data_sizes);
}

void
arraylist_add(struct arraylist *al, void const *new, size_t size)
{
	if (al->size >= al->cap) {
		al->cap *= 2;
		al->data = realloc(al->data, sizeof(void *) * al->cap);
		al->data_sizes = realloc(al->data_sizes, sizeof(void *) * al->cap);
	}

	al->data[al->size] = malloc(size);
	al->data_sizes[al->size] = size;
	memcpy(al->data[al->size], new, size);
	++al->size;
}

void
arraylist_rm(struct arraylist *al, size_t ind)
{
	free(al->data[ind]);
	size_t mv_size = (al->size-- - ind) * sizeof(void *);
	memmove(al->data + ind, al->data + ind + 1, mv_size);
}

void
arraylist_swap(struct arraylist *al, size_t ind_a, size_t ind_b)
{
	void *tmpp = al->data[ind_a];
	size_t tmps = al->data_sizes[ind_a];

	al->data[ind_a] = al->data[ind_b];
	al->data_sizes[ind_a] = al->data_sizes[ind_b];
	al->data[ind_b] = tmpp;
	al->data_sizes[ind_b] = tmps;
}

struct arraylist
arraylist_copy(struct arraylist const *al)
{
	struct arraylist cp = arraylist_create();

	for (size_t i = 0; i < al->size; ++i)
		arraylist_add(&cp, al->data[i], al->data_sizes[i]);

	return cp;
}

ssize_t
arraylist_find(struct arraylist const *al, void const *item, size_t size)
{
	for (ssize_t i = 0; i < al->size; ++i) {
		if (al->data_sizes[i] == size && !memcmp(al->data[i], item, size))
			return i;
	}

	return -1;
}

struct string
string_create(void)
{
	return (struct string){
		.data = malloc(1),
		.len = 0,
		.cap = 1,
	};
}

void
string_destroy(struct string *s)
{
	free(s->data);
}

void
string_push_ch(struct string *s, char ch)
{
	if (s->len >= s->cap) {
		s->cap *= 2;
		s->data = realloc(s->data, s->cap);
	}

	s->data[s->len++] = ch;
}

void
string_push_str(struct string *s, char const *str)
{
	for (char const *c = str; *c; ++c)
		string_push_ch(s, *c);
}

char *
string_to_str(struct string const *s)
{
	return strndup(s->data, s->len);
}

char const *
fileext(char const *path)
{
	char const *ext = strrchr(path, '.');
	return ext && ext != path ? ext + 1 : "\0";
}
