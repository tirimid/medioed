#include "conf.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hl/hl_c.h"
#include "hl/hl_md.h"
#include "hl/hl_sh.h"
#include "hl/hl_rs.h"
#include "hl/hl_s.h"
#include "mode/mode_c.h"
#include "mode/mode_md.h"
#include "mode/mode_sh.h"
#include "mode/mode_rs.h"
#include "mode/mode_s.h"
#include "util.h"

// binds.
int const conf_bind_quit[] = {K_CTL('x'), K_CTL('c'), -1};
int const conf_bind_chgfwd_frame[] = {K_CTL('x'), 'b', -1};
int const conf_bind_chgback_frame[] = {K_CTL('c'), 'b', -1};
int const conf_bind_focus_frame[] = {K_CTL('c'), 'f', -1};
int const conf_bind_kill_frame[] = {K_CTL('x'), 'k', -1};
int const conf_bind_open_file[] = {K_CTL('x'), K_CTL('f'), -1};
int const conf_bind_save_file[] = {K_CTL('x'), K_CTL('s'), -1};
int const conf_bind_navfwd_ch[] = {K_CTL('f'), -1};
int const conf_bind_navfwd_word[] = {K_META('f'), -1};
int const conf_bind_navfwd_page[] = {K_CTL('v'), -1};
int const conf_bind_navback_ch[] = {K_CTL('b'), -1};
int const conf_bind_navback_word[] = {K_META('b'), -1};
int const conf_bind_navback_page[] = {K_META('v'), -1};
int const conf_bind_navdown[] = {K_CTL('n'), -1};
int const conf_bind_navup[] = {K_CTL('p'), -1};
int const conf_bind_navln_start[] = {K_CTL('a'), -1};
int const conf_bind_navln_end[] = {K_CTL('e'), -1};
int const conf_bind_navgoto[] = {K_META('g'), K_META('g'), -1};
int const conf_bind_delfwd_ch[] = {K_CTL('d'), -1};
int const conf_bind_delback_ch[] = {K_BACKSPC, -1};
int const conf_bind_delback_word[] = {K_META(K_BACKSPC), -1};
int const conf_bind_chgmode_global[] = {K_CTL('c'), K_CTL('g'), 'g', -1};
int const conf_bind_chgmode_local[] = {K_CTL('c'), K_CTL('g'), 'l', -1};
int const conf_bind_create_scrap[] = {K_CTL('c'), 'n', -1};
int const conf_bind_newline[] = {K_RET, -1};
int const conf_bind_focus[] = {K_CTL('l'), -1};
int const conf_bind_kill[] = {K_CTL('k'), -1};
int const conf_bind_paste[] = {K_CTL('y'), -1};
int const conf_bind_undo[] = {K_CTL('x'), 'u', -1};
int const conf_bind_copy[] = {K_CTL('c'), K_SPC, K_META('w'), -1};
int const conf_bind_ncopy[] = {K_CTL('c'), K_SPC, K_META('n'), -1};

// language mode extensions.
static char const *ext_c[] = {"c", "h", NULL};
static char const *ext_md[] = {"md", NULL};
static char const *ext_sh[] = {"sh", NULL};
static char const *ext_rs[] = {"rs", NULL};
static char const *ext_s[] = {"s", "asm", "S", NULL};

// highlight table.
struct highlight const conf_htab[] = {
	{
		.localmode = "c",
		.find = hl_c_find,
	},
	{
		.localmode = "md",
		.find = hl_md_find,
	},
	{
		.localmode = "sh",
		.find = hl_sh_find,
	},
	{
		.localmode = "rs",
		.find = hl_rs_find,
	},
	{
		.localmode = "s",
		.find = hl_s_find,
	},
};
size_t const conf_htab_size = ARRAYSIZE(conf_htab);

// margin table.
struct margin const conf_mtab[] = {
	{
		.col = 80,
		.wch = L'|',
		.attr = A_WHITE | A_BGOF(CONF_A_NORM),
	},
	{
		.col = 110,
		.wch = L'|',
		.attr = A_RED | A_BGOF(CONF_A_NORM),
	},
};
size_t const conf_mtab_size = ARRAYSIZE(conf_mtab);

// language mode table.
struct mode const conf_lmtab[] = {
	{
		.name = "c",
		.init = mode_c_init,
		.quit = mode_c_quit,
		.update = mode_c_update,
		.keypress = mode_c_keypress,
	},
	{
		.name = "md",
		.init = mode_md_init,
		.quit = mode_md_quit,
		.update = mode_md_update,
		.keypress = mode_md_keypress,
	},
	{
		.name = "sh",
		.init = mode_sh_init,
		.quit = mode_sh_quit,
		.update = mode_sh_update,
		.keypress = mode_sh_keypress,
	},
	{
		.name = "rs",
		.init = mode_rs_init,
		.quit = mode_rs_quit,
		.update = mode_rs_update,
		.keypress = mode_rs_keypress,
	},
	{
		.name = "s",
		.init = mode_s_init,
		.quit = mode_s_quit,
		.update = mode_s_update,
		.keypress = mode_s_keypress,
	},
};
size_t const conf_lmtab_size = ARRAYSIZE(conf_lmtab);

// mode extension table.
struct modeext const conf_metab[] = {
	{
		.exts = ext_c,
		.mode = "c",
	},
	{
		.exts = ext_md,
		.mode = "md",
	},
	{
		.exts = ext_sh,
		.mode = "sh",
	},
	{
		.exts = ext_rs,
		.mode = "rs",
	},
	{
		.exts = ext_rs,
		.mode = "s",
	},
};
size_t const conf_metab_size = ARRAYSIZE(conf_metab);
