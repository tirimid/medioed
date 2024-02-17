#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

STK_DEF_IMPL(unsigned)

char const *
file_ext(char const *path)
{
	char const *ext = strrchr(path, '.');
	return ext && ext != path ? ext + 1 : "\0";
}

int mk_dir_rec(char const *dir)
{
	char *wd = strdup(dir);
	size_t wd_len = strlen(wd);
	if (!wd_len) {
		free(wd);
		return 1;
	}
	
	for (size_t i = 0, len = 0; i < wd_len; ++i) {
		if (wd[i] != '/') {
			++len;
			continue;
		}
		
		if (!len)
			continue;
		
		wd[i] = 0;
		struct stat s;
		if (!stat(wd, &s) && S_ISDIR(s.st_mode)) {
			wd[i] = '/';
			continue;
		}
		
		mode_t pm = umask(0);
		int rc = mkdir(wd, 0755);
		umask(pm);
		
		wd[i] = '/';
		
		if (rc) {
			free(wd);
			return 1;
		}

		len = 0;
	}
	
	free(wd);
	return 0;
}

int
mk_file(char const *path)
{
	struct stat s;
	if (!stat(path, &s))
		return !S_ISREG(s.st_mode);
	
	char *dir = strdup(path);
	if (strrchr(dir, '/'))
		*(strrchr(dir, '/') + 1) = 0;
	
	int rc = mk_dir_rec(dir);
	free(dir);
	if (rc)
		return 1;
	
	FILE *fp = fopen(path, "w");
	if (!fp)
		return 1;
	fclose(fp);
	
	// generally will be caused if the user tried to use `-c` when opening a
	// directory, like `$ medioed hello/`.
	if (stat(path, &s) || !S_ISREG(s.st_mode))
		return 1;
	
	return 0;
}

int
strncmp_case_insen(char const *a, char const *b, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		if (!a[i] && !b[i])
			break;
		else if (isalpha(a[i]) && isalpha(b[i])) {
			char a_i = tolower(a[i]), b_i = tolower(b[i]);
			if (a_i > b_i)
				return 1;
			else if (a_i < b_i)
				return -1;
		} else if (a[i] > b[i])
			return 1;
		else if (a[i] < b[i])
			return -1;
	}
	
	return 0;
}
