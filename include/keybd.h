#ifndef KEYBD_H__
#define KEYBD_H__

#define KEYBD_IGNORE 0xfedc1234

void keybd_init(void);
void keybd_quit(void);
void keybd_bind(char const *keyseq, void (*fn)(void));
void keybd_unbind(char const *keyseq);
int keybd_await_input(void);

#endif
