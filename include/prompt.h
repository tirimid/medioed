#ifndef PROMPT_H
#define PROMPT_H

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

void prompt_show(wchar_t const *msg);
wchar_t *prompt_ask(wchar_t const *msg, void (*comp)(wchar_t **, size_t *, size_t *));
int prompt_yes_no(wchar_t const *msg, bool deflt);

// completion functions for use in `comp` for `prompt_ask()`.
void prompt_comp_path(wchar_t **resp, size_t *rlen, size_t *csr);

#endif
