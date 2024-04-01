#include "conf.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hl/hl_c.h"
#include "hl/hl_cc.h"
#include "hl/hl_cs.h"
#include "hl/hl_html.h"
#include "hl/hl_lua.h"
#include "hl/hl_md.h"
#include "hl/hl_rs.h"
#include "hl/hl_s.h"
#include "hl/hl_sh.h"
#include "mode/mode_c.h"
#include "mode/mode_cc.h"
#include "mode/mode_cs.h"
#include "mode/mode_html.h"
#include "mode/mode_lua.h"
#include "mode/mode_md.h"
#include "mode/mode_rs.h"
#include "mode/mode_s.h"
#include "mode/mode_sh.h"
#include "util.h"

// binds.
int const conf_bind_quit[] = {K_CTL('x'), K_CTL('c'), -1};
int const conf_bind_chg_frame_fwd[] = {K_CTL('x'), 'b', -1};
int const conf_bind_chg_frame_back[] = {K_CTL('c'), 'b', -1};
int const conf_bind_focus_frame[] = {K_CTL('c'), 'f', -1};
int const conf_bind_kill_frame[] = {K_CTL('x'), 'k', -1};
int const conf_bind_open_file[] = {K_CTL('x'), K_CTL('f'), -1};
int const conf_bind_save_file[] = {K_CTL('x'), K_CTL('s'), -1};
int const conf_bind_nav_fwd_ch[] = {K_CTL('f'), -1};
int const conf_bind_nav_fwd_word[] = {K_META('f'), -1};
int const conf_bind_nav_fwd_page[] = {K_CTL('v'), -1};
int const conf_bind_nav_back_ch[] = {K_CTL('b'), -1};
int const conf_bind_nav_back_word[] = {K_META('b'), -1};
int const conf_bind_nav_back_page[] = {K_META('v'), -1};
int const conf_bind_nav_down[] = {K_CTL('n'), -1};
int const conf_bind_nav_up[] = {K_CTL('p'), -1};
int const conf_bind_nav_ln_start[] = {K_CTL('a'), -1};
int const conf_bind_nav_ln_end[] = {K_CTL('e'), -1};
int const conf_bind_nav_goto[] = {K_META('g'), K_META('g'), -1};
int const conf_bind_del_fwd_ch[] = {K_CTL('d'), -1};
int const conf_bind_del_back_ch[] = {K_BACKSPC, -1};
int const conf_bind_del_back_word[] = {K_META(K_BACKSPC), -1};
int const conf_bind_chg_mode_global[] = {K_CTL('c'), K_META('g'), 'g', -1};
int const conf_bind_chg_mode_local[] = {K_CTL('c'), K_META('g'), 'l', -1};
int const conf_bind_create_scrap[] = {K_CTL('c'), 'n', -1};
int const conf_bind_new_line[] = {K_RET, -1};
int const conf_bind_focus[] = {K_CTL('l'), -1};
int const conf_bind_kill[] = {K_CTL('k'), -1};
int const conf_bind_paste[] = {K_CTL('y'), -1};
int const conf_bind_undo[] = {K_CTL('x'), 'u', -1};
int const conf_bind_copy[] = {K_CTL('c'), K_SPC, K_META('w'), -1};
int const conf_bind_ncopy[] = {K_CTL('c'), K_SPC, K_META('n'), -1};
int const conf_bind_find_lit[] = {K_CTL('s'), 'l', -1};
int const conf_bind_mac_begin[] = {K_F(3), -1};
int const conf_bind_mac_end[] = {K_F(4), -1};
int const conf_bind_toggle_mono[] = {K_CTL('c'), 'm', -1};
int const conf_bind_read_man_word[] = {K_CTL('h'), K_CTL('d'), -1};
int const conf_bind_file_exp[] = {K_CTL('c'), 'd', -1};

// language mode extensions.
static char const *ext_c[] = {"c", "h", NULL};
static char const *ext_cc[] = {"cc", "hh", NULL};
static char const *ext_cs[] = {"cs", NULL};
static char const *ext_html[] = {"html", NULL};
static char const *ext_lua[] = {"lua", NULL};
static char const *ext_md[] = {"md", NULL};
static char const *ext_rs[] = {"rs", NULL};
static char const *ext_s[] = {"s", "S", NULL};
static char const *ext_sh[] = {"sh", NULL};

// highlight table.
struct highlight const conf_htab[] = {
	{
		.local_mode = "c",
		.find = hl_c_find,
	},
	{
		.local_mode = "cc",
		.find = hl_cc_find,
	},
	{
		.local_mode = "cs",
		.find = hl_cs_find,
	},
	{
		.local_mode = "html",
		.find = hl_html_find,
	},
	{
		.local_mode = "lua",
		.find = hl_lua_find,
	},
	{
		.local_mode = "md",
		.find = hl_md_find,
	},
	{
		.local_mode = "rs",
		.find = hl_rs_find,
	},
	{
		.local_mode = "s",
		.find = hl_s_find,
	},
	{
		.local_mode = "sh",
		.find = hl_sh_find,
	},
};
size_t const conf_htab_size = ARRAY_SIZE(conf_htab);

// margin table.
struct margin const conf_mtab[] = {
	{
		.col = 80,
		.wch = L'|',
		.fg = 241,
		.bg = CONF_A_NORM_BG,
	},
	{
		.col = 110,
		.wch = L'|',
		.fg = 248,
		.bg = CONF_A_NORM_BG,
	},
};
size_t const conf_mtab_size = ARRAY_SIZE(conf_mtab);

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
		.name = "cc",
		.init = mode_cc_init,
		.quit = mode_cc_quit,
		.update = mode_cc_update,
		.keypress = mode_cc_keypress,
	},
	{
		.name = "cs",
		.init = mode_cs_init,
		.quit = mode_cs_quit,
		.update = mode_cs_update,
		.keypress = mode_cs_keypress,
	},
	{
		.name = "html",
		.init = mode_html_init,
		.quit = mode_html_quit,
		.update = mode_html_update,
		.keypress = mode_html_keypress,
	},
	{
		.name = "lua",
		.init = mode_lua_init,
		.quit = mode_lua_quit,
		.update = mode_lua_update,
		.keypress = mode_lua_keypress,
	},
	{
		.name = "md",
		.init = mode_md_init,
		.quit = mode_md_quit,
		.update = mode_md_update,
		.keypress = mode_md_keypress,
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
	{
		.name = "sh",
		.init = mode_sh_init,
		.quit = mode_sh_quit,
		.update = mode_sh_update,
		.keypress = mode_sh_keypress,
	},
};
size_t const conf_lmtab_size = ARRAY_SIZE(conf_lmtab);

// mode extension table.
struct mode_ext const conf_metab[] = {
	{
		.exts = ext_c,
		.mode = "c",
	},
	{
		.exts = ext_cc,
		.mode = "cc",
	},
	{
		.exts = ext_cs,
		.mode = "cs",
	},
	{
		.exts = ext_html,
		.mode = "html",
	},
	{
		.exts = ext_lua,
		.mode = "lua",
	},
	{
		.exts = ext_md,
		.mode = "md",
	},
	{
		.exts = ext_rs,
		.mode = "rs",
	},
	{
		.exts = ext_s,
		.mode = "s",
	},
	{
		.exts = ext_sh,
		.mode = "sh",
	},
};
size_t const conf_metab_size = ARRAY_SIZE(conf_metab);
