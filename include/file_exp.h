#ifndef FILE_EXP_H
#define FILE_EXP_H

#include <stddef.h>

enum file_exp_rc {
	FER_SUCCESS = 0,
	FER_FAIL,
	FER_QUIT,
};

enum file_exp_rc file_exp_open(char *out_path, size_t n, char const *dir);

#endif
