#include "conf.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mode/mode_c.h"
#include "mode/mode_sh.h"
#include "util.h"

int const conf_bind_quit[] = {K_CTL('x'), K_CTL('c'), -1};
int const conf_bind_chgfwd_frame[] = {K_CTL('x'), 'b', -1};
int const conf_bind_chgback_frame[] = {K_CTL('c'), 'b', -1};
int const conf_bind_focus_frame[] = {K_CTL('c'), 'f', -1};
int const conf_bind_kill_frame[] = {K_CTL('x'), 'k', -1};
int const conf_bind_open_file[] = {K_CTL('x'), K_CTL('f'), -1};
int const conf_bind_save_file[] = {K_CTL('x'), K_CTL('s'), -1};
int const conf_bind_navfwd_ch[] = {K_CTL('f'), -1};
int const conf_bind_navfwd_word[] = {K_META('f'), -1};
int const conf_bind_navback_ch[] = {K_CTL('b'), -1};
int const conf_bind_navback_word[] = {K_META('b'), -1};
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

struct highlight conf_htab[] = {
#include "highlight/hl_c.h"
#include "highlight/hl_sh.h"
};
size_t const conf_htab_size = sizeof(conf_htab) / sizeof(conf_htab[0]);

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
size_t const conf_mtab_size = sizeof(conf_mtab) / sizeof(conf_mtab[0]);

struct mode const conf_lmtab[] = {
	{
		.name = "c",
		.init = mode_c_init,
		.quit = mode_c_quit,
		.keypress = mode_c_keypress,
	},
	{
		.name = "sh",
		.init = mode_sh_init,
		.quit = mode_sh_quit,
		.keypress = mode_sh_keypress,
	},
};
size_t const conf_lmtab_size = sizeof(conf_lmtab) / sizeof(conf_lmtab[0]);

int
conf_init(void)
{
	for (size_t i = 0; i < conf_htab_size; ++i) {
		struct highlight *hl = &conf_htab[i];

		int rc;
		uint64_t flags = PCRE2_MULTILINE;
		PCRE2_SIZE erroff;
		PCRE2_SPTR pat = (PCRE2_SPTR)hl->re_str;
		if (!(hl->re = pcre2_compile(pat, wcslen(hl->re_str), flags, &rc, &erroff, NULL))) {
			fprintf(stderr, "failed to compile regex: '%ls'!\n", hl->re_str);
			return 1;
		}
	}
	
	return 0;
}

void
conf_quit(void)
{
	for (size_t i = 0; i < conf_htab_size; ++i)
		pcre2_code_free(conf_htab[i].re);
}
