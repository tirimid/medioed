#ifndef UTIL_H__
#define UTIL_H__

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <unistd.h>

#define NOTHING__

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(min, n, max) MIN(MAX((min), (n)), (max))
#define ABS(n) ((n) < 0 ? -(n) : (n))
#define SIGN(n) ((n) / ABS(n))

#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

#define K_CTL(k) (k - 'a' + 1)
#define K_META(k) 27, k
#define K_BACKSPC 127
#define K_RET 13
#define K_TAB 9
#define K_SPC 32

#define VEC_DEFPROTO_EX(t, acclevel) \
	struct vec_##t { \
		t *data; \
		size_t size, cap; \
	}; \
	acclevel struct vec_##t vec_##t##_create(void); \
	acclevel void vec_##t##_destroy(struct vec_##t *v); \
	acclevel void vec_##t##_add(struct vec_##t *v, t *new); \
	acclevel void vec_##t##_rm(struct vec_##t *v, size_t ind); \
	acclevel void vec_##t##_swap(struct vec_##t *v, size_t a, size_t b);

#define VEC_DEFIMPL_EX(t, acclevel) \
	acclevel struct vec_##t \
	vec_##t##_create(void) \
	{ \
		return (struct vec_##t){ \
			.data = malloc(sizeof(t)), \
			.size = 0, \
			.cap = 1, \
		}; \
	} \
	acclevel void \
	vec_##t##_destroy(struct vec_##t *v) \
	{ \
		free(v->data); \
	} \
	acclevel void \
	vec_##t##_add(struct vec_##t *v, t *new) \
	{ \
		if (v->size >= v->cap) { \
			v->cap *= 2; \
			v->data = realloc(v->data, sizeof(t) * v->cap); \
		} \
		v->data[v->size++] = *new; \
	} \
	acclevel void \
	vec_##t##_rm(struct vec_##t *v, size_t ind) \
	{ \
		memmove(&v->data[ind], &v->data[ind + 1], sizeof(t) * (v->size - ind - 1)); \
		--v->size; \
	} \
	acclevel void \
	vec_##t##_swap(struct vec_##t *v, size_t a, size_t b) \
	{ \
		t tmp = v->data[a]; \
		v->data[a] = v->data[b]; \
		v->data[b] = tmp; \
	}

#define STK_DEFPROTO_EX(t, acclevel) \
	struct stk_##t { \
		t *data; \
		size_t size, cap; \
	}; \
	acclevel struct stk_##t stk_##t##_create(void); \
	acclevel void stk_##t##_destroy(struct stk_##t *s); \
	acclevel void stk_##t##_push(struct stk_##t *s, t *new); \
	acclevel t *stk_##t##_pop(struct stk_##t *s); \
	acclevel t const *stk_##t##_peek(struct stk_##t const *s);

#define STK_DEFIMPL_EX(t, acclevel) \
	acclevel struct stk_##t \
	stk_##t##_create(void) \
	{ \
		return (struct stk_##t){ \
			.data = malloc(sizeof(t)), \
			.size = 0, \
			.cap = 1, \
		}; \
	} \
	acclevel void \
	stk_##t##_destroy(struct stk_##t *s) \
	{ \
		free(s->data); \
	} \
	acclevel void \
	stk_##t##_push(struct stk_##t *s, t *new) \
	{ \
		if (s->size >= s->cap) { \
			s->cap *= 2; \
			s->data = realloc(s->data, sizeof(t) * s->cap); \
		} \
		s->data[s->size++] = *new; \
	} \
	acclevel t * \
	stk_##t##_pop(struct stk_##t *s) \
	{ \
		if (!s->size) \
			return NULL; \
		t *item = malloc(sizeof(t)); \
		memcpy(item, &s->data[--s->size], sizeof(t)); \
		return item; \
	} \
	acclevel t const * \
	stk_##t##_peek(struct stk_##t const *s) \
	{ \
		return !s->size ? NULL : &s->data[s->size - 1]; \
	}

#define VEC_DEFPROTO(t) VEC_DEFPROTO_EX(t, NOTHING__)
#define VEC_DEFIMPL(t) VEC_DEFIMPL_EX(t, NOTHING__)
#define VEC_DEFPROTO_STATIC(t) VEC_DEFPROTO_EX(t, static)
#define VEC_DEFIMPL_STATIC(t) VEC_DEFIMPL_EX(t, static)
#define STK_DEFPROTO(t) STK_DEFPROTO_EX(t, NOTHING__)
#define STK_DEFIMPL(t) STK_DEFIMPL_EX(t, NOTHING__)
#define STK_DEFPROTO_STATIC(t) STK_DEFPROTO_EX(t, static)
#define STK_DEFIMPL_STATIC(t) STK_DEFIMPL_EX(t, static)

STK_DEFPROTO(unsigned)

char const *fileext(char const *path);
int mkdirrec(char const *dir);
int mkfile(char const *path);

#endif
