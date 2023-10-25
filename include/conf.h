#ifndef CONF_H__
#define CONF_H__

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "draw.h"
#include "mode.h"

#define CONF_TABSIZE 8
#define CONF_GUTTER_LEFT 1
#define CONF_GUTTER_RIGHT 1
#define CONF_BUFMODMARK L"~~ (*)"
#define CONF_MNUM 4
#define CONF_MDENOM 7

#define CONF_A_GNORM (A_YELLOW | A_BBLACK)
#define CONF_A_GHIGH (A_BLACK | A_BYELLOW)
#define CONF_A_NORM (A_WHITE | A_BBLACK)
#define CONF_A_LINUM (A_YELLOW | A_BBLACK)
#define CONF_A_CURSOR (A_BLACK | A_BWHITE)

#define CONF_GREETNAME L"*greeter*"
#define CONF_SCRAPNAME L"*scrap*"

#define CONF_GREETTEXT \
	L"welcome to medioed, the (medio)cre text (ed)itor\n" \
	L"\n" \
	L"copyleft (c) Fuck Copyright\n" \
	L"\n" \
	L"this program is 100% free software, licensed under the GNU GPLv3\n" \
	L"the source is available at https://gitlab.com/tirimid/medioed\n"

struct highlight {
	char const **localmodes;
	int (*find)(wchar_t const *, size_t, size_t, size_t *, size_t *, uint16_t *);
};

struct margin {
	unsigned col;
	wchar_t wch;
	uint16_t attr;
};

struct modeext {
	char const **exts;
	char const *mode;
};

extern int const conf_bind_quit[];
extern int const conf_bind_chgfwd_frame[];
extern int const conf_bind_chgback_frame[];
extern int const conf_bind_focus_frame[];
extern int const conf_bind_kill_frame[];
extern int const conf_bind_open_file[];
extern int const conf_bind_save_file[];
extern int const conf_bind_navfwd_ch[];
extern int const conf_bind_navfwd_word[];
extern int const conf_bind_navfwd_page[];
extern int const conf_bind_navback_ch[];
extern int const conf_bind_navback_word[];
extern int const conf_bind_navback_page[];
extern int const conf_bind_navdown[];
extern int const conf_bind_navup[];
extern int const conf_bind_navln_start[];
extern int const conf_bind_navln_end[];
extern int const conf_bind_navgoto[];
extern int const conf_bind_delfwd_ch[];
extern int const conf_bind_delback_ch[];
extern int const conf_bind_delback_word[];
extern int const conf_bind_chgmode_global[];
extern int const conf_bind_chgmode_local[];
extern int const conf_bind_create_scrap[];
extern int const conf_bind_newline[];
extern int const conf_bind_focus[];

extern struct highlight const conf_htab[];
extern size_t const conf_htab_size;
extern struct margin const conf_mtab[];
extern size_t const conf_mtab_size;
extern struct mode const conf_lmtab[];
extern size_t const conf_lmtab_size;
extern struct modeext const conf_metab[];
extern size_t const conf_metab_size;

#endif
