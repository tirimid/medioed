#ifndef PROMPT_H__
#define PROMPT_H__

#include <stddef.h>

void prompt_show(char const *text);
char *prompt_ask(char const *text);
char *prompt_ask_buf(char out_buf[], size_t n, char const *text);

#endif
