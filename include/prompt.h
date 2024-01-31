#ifndef PROMPT_H__
#define PROMPT_H__

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

void prompt_show(wchar_t const *msg);
wchar_t *prompt_ask(wchar_t const *msg, void (*comp)(wchar_t **, size_t *, void *), void *cdata);
int prompt_yes_no(wchar_t const *msg, bool deflt);

void prompt_comp_path(wchar_t **resp, size_t *rlen, void *cdata);

#endif
