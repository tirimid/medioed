#ifndef KEYBD_H__
#define KEYBD_H__

#include <stddef.h>
#include <wchar.h>

#define KEYBD_IGNORE 0xfedc1234
#define KEYBD_MAXBINDLEN 32
#define KEYBD_MAXDPYLEN 7

void keybd_init(void);
void keybd_quit(void);
void keybd_bind(int const *keyseq, void (*fn)(void));
void keybd_organize(void);
wint_t keybd_awaitkey(void);
void keybd_keydpy(wchar_t *out, int const *kbuf, size_t nk);
int const *keybd_curbind(size_t *out_len);

#endif
