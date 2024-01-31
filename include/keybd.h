#ifndef KEYBD_H__
#define KEYBD_H__

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

#define KEYBD_IGNORE 0xfedc1234
#define KEYBD_MAX_BIND_LEN 32
#define KEYBD_MAX_DPY_LEN 7
#define KEYBD_MAX_MAC_LEN 512

void keybd_init(void);
void keybd_quit(void);
void keybd_bind(int const *key_seq, void (*fn)(void));
void keybd_organize(void);
void keybd_rec_mac_begin(void);
bool keybd_is_rec_mac(void);
void keybd_rec_mac_end(void);
void keybd_exec_mac(void);
bool keybd_is_exec_mac(void);
wint_t keybd_await_key_nb(void);
wint_t keybd_await_key(void);
void keybd_key_dpy(wchar_t *out, int const *kbuf, size_t nk);
int const *keybd_cur_bind(size_t *out_len);
int const *keybd_cur_mac(size_t *out_len);

#endif
