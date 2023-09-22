#ifndef PROMPT_H__
#define PROMPT_H__

#include <stddef.h>

void prompt_show(char const *text);
char *prompt_ask(char const *text, void (*complete)(char **, size_t *, void *), void *compdata);

void prompt_complete_path(char **resp, size_t *resp_len, void *data);

#endif
