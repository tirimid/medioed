#include "util.h"

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

STK_DEFIMPL(unsigned)

char const *
fileext(char const *path)
{
	char const *ext = strrchr(path, '.');
	return ext && ext != path ? ext + 1 : "\0";
}

int mkdirrec(char const *dir)
{
	char *wd = strdup(dir);
	size_t wdlen = strlen(wd);
	if (!wdlen) {
		free(wd);
		return 1;
	}
	
	for (size_t i = 0, len = 0; i < wdlen; ++i) {
		if (wd[i] != '/') {
			++len;
			continue;
		}
		
		if (!len)
			continue;
		
		wd[i] = 0;
		struct stat s;
		if (!stat(wd, &s) && S_ISDIR(s.st_mode))
			continue;
		
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
mkfile(char const *path)
{
	struct stat s;
	if (!stat(path, &s))
		return !S_ISREG(s.st_mode);
	
	char *dir = strdup(path);
	if (strrchr(dir, '/'))
		*(strrchr(dir, '/') + 1) = 0;
	
	int rc = mkdirrec(dir);
	free(dir);
	if (rc)
		return 1;
	
	// generally will be caused if the user tried to use `-c` when opening a
	// directory, like `$ medioed hello/`.
	if (!stat(path, &s) || !S_ISREG(s.st_mode))
		return 1;
	
	return 0;
}
