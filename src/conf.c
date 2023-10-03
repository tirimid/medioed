#include "conf.h"

#include <stdio.h>

#include "mode/mode_c.h"

#define ERRBUF_SIZE 512

struct highlight conf_htab[] = {
#include "highlight/hl_c.h"
};
size_t const conf_htab_size = sizeof(conf_htab) / sizeof(struct highlight);

// `pos` is zero-indexed.
struct margin conf_mtab[] = {
	{
		.pos = 80,
		.ch = '|',
		.fg = COLOR_WHITE,
		.bg = COLOR_BLACK,
	},
	{
		.pos = 110,
		.ch = '|',
		.fg = COLOR_RED,
		.bg = COLOR_BLACK,
	},
};
size_t const conf_mtab_size = sizeof(conf_mtab) / sizeof(struct margin);

struct mode conf_lmtab[] = {
	{
		.name = "c",
		.init = mode_c_init,
		.quit = mode_c_quit,
		.keypress = mode_c_keypress,
	},
};
size_t const conf_lmtab_size = sizeof(conf_lmtab) / sizeof(struct mode);

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

		int rc;
		uint64_t flags = PCRE2_ZERO_TERMINATED;
		PCRE2_SIZE erroff;
		PCRE2_SPTR pat = (PCRE2_SPTR)hl->re_str;
		if (!(hl->re = pcre2_compile(pat, flags, 0, &rc, &erroff, NULL))) {
			char errbuf[ERRBUF_SIZE];
			pcre2_get_error_message(rc, (PCRE2_UCHAR *)errbuf, ERRBUF_SIZE);

			fprintf(stderr, "failed to compile regex: '%s'!\n", hl->re_str);
			fprintf(stderr, "pcre2 error at %zu: '%s'!\n", erroff, errbuf);

			return 1;
		}

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
		struct highlight *hl = &conf_htab[i];

		pcre2_code_free(hl->re);
		free_pair(hl->colpair);
	}

	for (size_t i = 0; i < conf_mtab_size; ++i)
		free_pair(conf_mtab[i].colpair);
}
