#ifndef MODE_MODE_HTML_H
#define MODE_MODE_HTML_H

#include <wchar.h>

#include "frame.h"

void mode_html_init(struct frame *f);
void mode_html_quit(void);
void mode_html_update(void);
void mode_html_keypress(wint_t k);

#endif
