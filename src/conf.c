#include "conf.h"

struct highlight conf_htab[] = {
	{
		.re_str = "#[\\s\\S]*",
		.mode = "c",
		.scope = HIGHLIGHT_SCOPE_LINE,
		.bg = COLOR_BLACK,
		.fg = COLOR_CYAN,
	},
};
size_t const conf_htab_size = sizeof(conf_htab) / sizeof(struct highlight);

// `pos` is zero-indexed.
struct margin conf_mtab[] = {
	{
		.pos = 79,
		.ch = '|',
		.fg = COLOR_RED,
		.bg = COLOR_BLACK,
	},
};
size_t const conf_mtab_size = sizeof(conf_mtab) / sizeof(struct margin);

int conf_gnorm, conf_ghighlight;
int conf_norm, conf_linum, conf_cursor;

int
conf_init(void)
{
	conf_gnorm = alloc_pair(CONF_GNORM_FG, CONF_GNORM_BG);
	conf_ghighlight = alloc_pair(CONF_GHIGHLIGHT_FG, CONF_GHIGHLIGHT_BG);
	
	conf_norm = alloc_pair(CONF_NORM_FG, CONF_NORM_BG);
	conf_linum = alloc_pair(CONF_LINUM_FG, CONF_LINUM_BG);
	conf_cursor = alloc_pair(CONF_CURSOR_FG, CONF_CURSOR_BG);

	for (size_t i = 0; i < conf_htab_size; ++i) {
		struct highlight *hl = &conf_htab[i];
		int re_flags = REG_NEWLINE * (hl->scope == HIGHLIGHT_SCOPE_LINE);
		if (regcomp(&hl->re, hl->re_str, re_flags) != 0)
			return 1;

		hl->colpair = alloc_pair(hl->fg, hl->bg);
	}

	for (size_t i = 0; i < conf_mtab_size; ++i) {
		struct margin *m = &conf_mtab[i];
		m->colpair = alloc_pair(m->fg, m->bg);
	}

	return 0;
}

void
conf_quit(void)
{
	free_pair(conf_gnorm);
	free_pair(conf_ghighlight);
	
	free_pair(conf_norm);
	free_pair(conf_linum);
	free_pair(conf_cursor);

	for (size_t i = 0; i < conf_htab_size; ++i) {
		regfree(&conf_htab[i].re);
		free_pair(conf_htab[i].colpair);
	}

	for (size_t i = 0; i < conf_mtab_size; ++i)
		free_pair(conf_mtab[i].colpair);
}
