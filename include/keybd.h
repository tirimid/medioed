#ifndef KEYBD_H__
#define KEYBD_H__

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

#define KEYBD_IGNORE 0xfedc1234
#define KEYBD_MAXBINDLEN 32
#define KEYBD_MAXDPYLEN 7
#define KEYBD_MAXMACLEN 512

void keybd_init(void);
void keybd_quit(void);
void keybd_bind(int const *keyseq, void (*fn)(void));
void keybd_organize(void);
void keybd_recmac_begin(void);
bool keybd_isrecmac(void);
void keybd_recmac_end(void);
void keybd_execmac(void);
bool keybd_isexecmac(void);
wint_t keybd_awaitkey_nb(void);
wint_t keybd_awaitkey(void);
void keybd_keydpy(wchar_t *out, int const *kbuf, size_t nk);
int const *keybd_curbind(size_t *out_len);
int const *keybd_curmac(size_t *out_len);

#endif
