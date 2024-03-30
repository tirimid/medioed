#include "label.h"

#include <string.h>
#include <wctype.h>

#include <sys/ioctl.h>

#include "conf.h"
#include "draw.h"
#include "editor.h"
#include "keybd.h"
#include "util.h"

#define BIND_QUIT K_CTL('g')
#define BIND_NAV_DOWN K_CTL('n')
#define BIND_NAV_UP K_CTL('p')

static void draw_box(wchar_t const *name, wchar_t const *msg, size_t off, struct label_bounds const *bounds);

int
label_rebound(struct label_bounds *bounds, unsigned anchor_l, unsigned anchor_r,
              unsigned anchor_t)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);
	
	struct label_bounds new = {
		.sr = bounds->sr,
		.sc = bounds->sc,
	};
	
	// left-hand anchoring is preferred.
	if (anchor_l + bounds->sc <= ws.ws_col)
		new.pc = anchor_l;
	else if (anchor_r <= ws.ws_col + bounds->sc)
		new.pc = MAX((long)anchor_r - (long)bounds->sc + 1, 0);
	else {
		// could not get valid position with either horizontal anchor.
		return 1;
	}
	
	new.pr = anchor_t;
	while (new.pr > 0 && new.pr + bounds->sr > ws.ws_row)
		--new.pr;
	
	// could not get valid position with vertical anchor.
	if (new.pr + bounds->sr > ws.ws_row)
		return 1;
	
	*bounds = new;
	return 0;
}

int
label_show(wchar_t const *name, wchar_t const *msg,
           struct label_bounds const *bounds)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);
	
	// refuse to display if there isn't enough space to view the label at a
	// specific, set size.
	if (bounds->pc + bounds->sc > ws.ws_col
	    || bounds->pr + bounds->sr > ws.ws_row) {
		return 1;
	}
	
	// the bind indicator is not covered like with prompt.
	// thus, it should be redrawn as to not appear incorrect.
	editor_redraw();
	
	size_t off = 0;
	for (;;) {
		draw_box(name, msg, off, bounds);
		draw_refresh();
		
		wint_t k = keybd_await_key_nb();
		switch (k) {
		case WEOF:
		case BIND_QUIT:
			return 0;
		case BIND_NAV_DOWN: {
			size_t sv_off = off;
			
			while (msg[off] && msg[off] != L'\n')
				++off;
			
			if (!msg[off]) {
				off = sv_off;
				break;
			}
			
			// only advance if not on last line.
			if (!msg[++off]) {
				off = sv_off;
				break;
			}
			
			break;
		}
		case BIND_NAV_UP:
			if (off == 0)
				break;
			--off;
			while (off > 0 && msg[off - 1] != L'\n')
				--off;
			break;
		default:
			break;
		}
	}
}

static void
draw_box(wchar_t const *name, wchar_t const *msg, size_t off,
         struct label_bounds const *bounds)
{
	// write label title.
	size_t name_len = wcslen(name);
	for (unsigned i = 0; i < bounds->sc; ++i)
		draw_put_wch(bounds->pr, bounds->pc + i, i < name_len ? name[i] : L' ');
	draw_put_attr(bounds->pr, bounds->pc, CONF_A_GHIGH_FG, CONF_A_GHIGH_BG, bounds->sc);
	
	// fill label.
	draw_fill(bounds->pr + 1, bounds->pc, bounds->sr - 1, bounds->sc, L' ',
	          CONF_A_GHIGH_BG, CONF_A_GHIGH_FG);
	
	// write message.
	unsigned r = 1, c = 0;
	for (size_t i = off; msg[i]; ++i) {
		if (msg[i] == L'\n' || c >= bounds->sc) {
			c = 0;
			++r;
			continue;
		}
		
		if (r >= bounds->sr)
			break;
		
		// here, just ignore non-printing chars.
		// they are not needed, and nothing is lost if the user doesn't
		// know about their existence.
		if (msg[i] != L'\t' && !iswprint(msg[i]))
			continue;
		
		switch (msg[i]) {
		case L'\t':
			c += CONF_TAB_SIZE - c % CONF_TAB_SIZE;
			break;
		default:
			draw_put_wch(bounds->pr + r, bounds->pc + c, msg[i]);
			++c;
			break;
		}
	}
}
