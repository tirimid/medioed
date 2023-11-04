#include "util.h"

#include <string.h>

STK_DEFIMPL(long)

char const *
fileext(char const *path)
{
	char const *ext = strrchr(path, '.');
	return ext && ext != path ? ext + 1 : "\0";
}
