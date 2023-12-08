#ifndef MODE_HTML_H__
#define MODE_HTML_H__

#include <wchar.h>

#include "frame.h"

void mode_html_init(struct frame *f);
void mode_html_quit(void);
void mode_html_update(void);
void mode_html_keypress(wint_t k);

#endif
