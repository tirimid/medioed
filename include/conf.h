#ifndef CONF_H
#define CONF_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "draw.h"
#include "mode.h"

// text layout options.
#define CONF_TAB_SIZE 6
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

// scrap buffer options.
#define CONF_SCRAP_NAME L"*scrap*"

// manpage viewer options.
#define CONF_READ_MAN_TITLE L"*manpage viewer*"
#define CONF_READ_MAN_CMD "man -E ascii -P cat %w 2> /dev/null"
#define CONF_READ_MAN_SR 20
#define CONF_READ_MAN_SC 85

// file explorer options.
#define CONF_FILE_EXP_NAME L"*file explorer*"
#define CONF_FILE_EXP_SC 25

// greeter options.
#define CONF_GREET_NAME L"*greeter*"
#define CONF_GREET_TEXT \
	L"                    _ _                _\n" \
	L" _ __ ___   ___  __| (_) ___   ___  __| |\n" \
	L"| '_ ` _ \\ / _ \\/ _` | |/ _ \\ / _ \\/ _` |\n" \
	L"| | | | | |  __/ (_| | | (_) |  __/ (_| |\n" \
	L"|_| |_| |_|\\___|\\__,_|_|\\___/ \\___|\\__,_|\n" \
	L"\n" \
	L"Welcome to medioed, the (Medio)cre Text (Ed)itor inspired by GNU Emacs.\n" \
	L"\n" \
	L"Medioed has always been, and will always be, Free Software, licensed under the\n" \
	L"GNU GPLv3, for the benefit of users and anyone wishing to study the source. I\n" \
	L"created medioed for my own personal use, but you are free to use it on your own\n" \
	L"machines as well.\n" \
	L"\n" \
	L"Additional resources:\n" \
	L"\n" \
	L"* Source code: https://github.com/tirimid/medioed\n" \
	L"* Documentation: https://tirimid.net/software/medioed.html\n" \
	L"\n" \
	L"(The greeter logo seen above was generated with the use of Figlet)\n"

struct highlight
{
	char const *local_mode;
	int (*find)(struct buf const *, size_t, size_t *, size_t *, uint8_t *, uint8_t *);
};

struct margin
{
	unsigned col;
	wchar_t wch;
	uint8_t fg, bg;
};

struct mode_ext
{
	char const **exts;
	char const *localmode, *globalmode;
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
extern int const conf_bind_read_man_word[];
extern int const conf_bind_file_exp[];

extern struct highlight const conf_htab[];
extern size_t const conf_htab_size;
extern struct margin const conf_mtab[];
extern size_t const conf_mtab_size;
extern struct mode const conf_lmtab[];
extern size_t const conf_lmtab_size;
extern struct mode_ext const conf_metab[];
extern size_t const conf_metab_size;

#endif
