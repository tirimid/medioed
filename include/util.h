#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <unistd.h>

#define NOTHING

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(min, n, max) MIN(MAX((min), (n)), (max))
#define ABS(n) ((n) < 0 ? -(n) : (n))
#define SIGN(n) ((n) / ABS(n))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define K_CTL(k) (k - 'a' + 1)
#define K_F(n) 27, 79, (79 + n)
#define K_META(k) 27, k
#define K_BACKSPC 127
#define K_ESC 27
#define K_RET 13
#define K_SPC 32
#define K_TAB 9

#define VEC_DEF_PROTO_EX(t, name, acc_level) \
	struct vec_##name \
	{ \
		t *data; \
		size_t size, cap; \
	}; \
	acc_level struct vec_##name vec_##name##_create(void); \
	acc_level void vec_##name##_destroy(struct vec_##name *v); \
	acc_level void vec_##name##_add(struct vec_##name *v, t *new); \
	acc_level void vec_##name##_rm(struct vec_##name *v, size_t ind); \
	acc_level void vec_##name##_swap(struct vec_##name *v, size_t a, size_t b);

#define VEC_DEF_IMPL_EX(t, name, acc_level) \
	acc_level struct vec_##name \
	vec_##name##_create(void) \
	{ \
		return (struct vec_##name) \
		{ \
			.data = malloc(sizeof(t)), \
			.size = 0, \
			.cap = 1, \
		}; \
	} \
	acc_level void \
	vec_##name##_destroy(struct vec_##name *v) \
	{ \
		free(v->data); \
	} \
	acc_level void \
	vec_##name##_add(struct vec_##name *v, t *new) \
	{ \
		if (v->size >= v->cap) \
		{ \
			v->cap *= 2; \
			v->data = realloc(v->data, sizeof(t) * v->cap); \
		} \
		v->data[v->size++] = *new; \
	} \
	acc_level void \
	vec_##name##_rm(struct vec_##name *v, size_t ind) \
	{ \
		memmove(&v->data[ind], \
		        &v->data[ind + 1], \
		        sizeof(t) * (v->size - ind - 1)); \
		--v->size; \
	} \
	acc_level void \
	vec_##name##_swap(struct vec_##name *v, size_t a, size_t b) \
	{ \
		t tmp = v->data[a]; \
		v->data[a] = v->data[b]; \
		v->data[b] = tmp; \
	}

#define VEC_DEF_PROTO(t, name) VEC_DEF_PROTO_EX(t, name, NOTHING)
#define VEC_DEF_IMPL(t, name) VEC_DEF_IMPL_EX(t, name, NOTHING)
#define VEC_DEF_PROTO_STATIC(t, name) VEC_DEF_PROTO_EX(t, name, static)
#define VEC_DEF_IMPL_STATIC(t, name) VEC_DEF_IMPL_EX(t, name, static)

VEC_DEF_PROTO(char *, str)

char const *file_ext(char const *path);
int mk_dir_rec(char const *dir);
int mk_file(char const *path);
bool is_path_same(char const *pa, char const *pb);

#endif
