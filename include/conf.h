#ifndef CONF_H
#define CONF_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "draw.h"
#include "mode.h"

// text layout options.
#define CONF_TAB_SIZE 8
#define CONF_GUTTER_LEFT 1
#define CONF_GUTTER_RIGHT 1
#define CONF_MNUM 4
#define CONF_MDENOM 7

// master color options.
#define CONF_A_GNORM_FG 183
#define CONF_A_GNORM_BG 232
#define CONF_A_GHIGH_FG CONF_A_GNORM_BG
#define CONF_A_GHIGH_BG CONF_A_GNORM_FG
#define CONF_A_NORM_FG 253
#define CONF_A_NORM_BG 233
#define CONF_A_LINUM_FG 245
#define CONF_A_LINUM_BG 232
#define CONF_A_CURSOR_FG 0
#define CONF_A_CURSOR_BG 15

// theme color options.
#define CONF_A_ACCENT_1_FG 183
#define CONF_A_ACCENT_1_BG CONF_A_NORM_BG
#define CONF_A_ACCENT_2_FG 176
#define CONF_A_ACCENT_2_BG CONF_A_NORM_BG
#define CONF_A_ACCENT_3_FG 229
#define CONF_A_ACCENT_3_BG CONF_A_NORM_BG
#define CONF_A_ACCENT_4_FG 212
#define CONF_A_ACCENT_4_BG CONF_A_NORM_BG
#define CONF_A_STRING_FG 182
#define CONF_A_STRING_BG 52
#define CONF_A_SPECIAL_FG 247
#define CONF_A_SPECIAL_BG CONF_A_NORM_BG
#define CONF_A_COMMENT_FG 245
#define CONF_A_COMMENT_BG 235

#define CONF_MARK_MOD L"[~*]"
#define CONF_MARK_MONO L"[M!]"

#define CONF_GREET_NAME L"*greeter*"
#define CONF_SCRAP_NAME L"*scrap*"

#define CONF_GREET_TEXT \
	L"                    _ _                _\n" \
	L" _ __ ___   ___  __| (_) ___   ___  __| |\n" \
	L"| '_ ` _ \\ / _ \\/ _` | |/ _ \\ / _ \\/ _` |\n" \
	L"| | | | | |  __/ (_| | | (_) |  __/ (_| |\n" \
	L"|_| |_| |_|\\___|\\__,_|_|\\___/ \\___|\\__,_|\n" \
	L"\n" \
	L"Welcome to medioed, the (Medio)cre Text (Ed)itor. You can find more information\n" \
	L"regarding this program at https://tirimid.net/software/medioed.html.\n" \
	L"\n" \
	L"Copyleft (c) Fuck Copyright\n" \
	L"\n" \
	L"This program is 100% free software, licensed under the GNU GPLv3. The source is\n" \
	L"available at https://gitlab.com/tirimid/medioed.\n"

struct highlight {
	char const *local_mode;
	int (*find)(wchar_t const *, size_t, size_t, size_t *, size_t *, uint8_t *, uint8_t *);
};

struct margin {
	unsigned col;
	wchar_t wch;
	uint8_t fg, bg;
};

struct mode_ext {
	char const **exts;
	char const *mode;
};

extern int const conf_bind_quit[];
extern int const conf_bind_chg_frame_fwd[];
extern int const conf_bind_chg_frame_back[];
extern int const conf_bind_focus_frame[];
extern int const conf_bind_kill_frame[];
extern int const conf_bind_open_file[];
extern int const conf_bind_save_file[];
extern int const conf_bind_nav_fwd_ch[];
extern int const conf_bind_nav_fwd_word[];
extern int const conf_bind_nav_fwd_page[];
extern int const conf_bind_nav_back_ch[];
extern int const conf_bind_nav_back_word[];
extern int const conf_bind_nav_back_page[];
extern int const conf_bind_nav_down[];
extern int const conf_bind_nav_up[];
extern int const conf_bind_nav_ln_start[];
extern int const conf_bind_nav_ln_end[];
extern int const conf_bind_nav_goto[];
extern int const conf_bind_del_fwd_ch[];
extern int const conf_bind_del_back_ch[];
extern int const conf_bind_del_back_word[];
extern int const conf_bind_chg_mode_global[];
extern int const conf_bind_chg_mode_local[];
extern int const conf_bind_create_scrap[];
extern int const conf_bind_new_line[];
extern int const conf_bind_focus[];
extern int const conf_bind_kill[];
extern int const conf_bind_paste[];
extern int const conf_bind_undo[];
extern int const conf_bind_copy[];
extern int const conf_bind_ncopy[];
extern int const conf_bind_find_lit[];
extern int const conf_bind_mac_begin[];
extern int const conf_bind_mac_end[];
extern int const conf_bind_toggle_mono[];

extern struct highlight const conf_htab[];
extern size_t const conf_htab_size;
extern struct margin const conf_mtab[];
extern size_t const conf_mtab_size;
extern struct mode const conf_lmtab[];
extern size_t const conf_lmtab_size;
extern struct mode_ext const conf_metab[];
extern size_t const conf_metab_size;

#endif
