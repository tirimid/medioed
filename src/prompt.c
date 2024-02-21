#include "prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <dirent.h>
#include <sys/ioctl.h>

#include "conf.h"
#include "draw.h"
#include "keybd.h"
#include "util.h"

#define BIND_CANCEL K_CTL('g')
#define BIND_NAV_FWD K_CTL('f')
#define BIND_NAV_BACK K_CTL('b')
#define BIND_DEL K_BACKSPC
#define BIND_COMPLETE K_TAB

static void draw_box(wchar_t const *text);

void
prompt_show(wchar_t const *msg)
{
	draw_box(msg);
	draw_refresh();
	
	for (;;) {
		wint_t k = keybd_await_key_nb();
		if (k == WEOF || k == L'q' || k == L'Q')
			break;
	}
}

wchar_t *
prompt_ask(wchar_t const *msg, void (*comp)(wchar_t **, size_t *, size_t *))
{
	draw_box(msg);

	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	// determine where the response should be rendered.
	unsigned render_row = ws.ws_row - 1, render_col = 0;
	for (wchar_t const *c = msg; *c; ++c) {
		if (*c == L'\n' || ++render_col > ws.ws_col)
			render_col = 0;
	}

	// a faux cursor is drawn before entering the keyboard loop, so that it
	// doesn't look like it spontaneously appears upon a keypress.
	draw_put_attr(render_row, render_col, CONF_A_GHIGH_FG, CONF_A_GHIGH_BG, 1);
	draw_refresh();

	wchar_t *resp = malloc(sizeof(wchar_t));
	size_t resp_len = 0;
	size_t csr = 0, draw_start = 0;

	wint_t k;
	while ((k = keybd_await_key_nb()) != K_RET) {
		// gather response.
		if (k == BIND_CANCEL || k == WEOF) {
			free(resp);
			return NULL;
		} else if (k == BIND_NAV_FWD)
			csr += csr < resp_len;
		else if (k == BIND_NAV_BACK)
			csr -= csr > 0;
		else if (k == BIND_DEL) {
			if (csr > 0) {
				memmove(resp + csr - 1, resp + csr, sizeof(wchar_t) * (resp_len - csr));
				--resp_len;
				--csr;
			}
		} else if (k == BIND_COMPLETE) {
			if (comp)
				comp(&resp, &resp_len, &csr);
		} else {
			resp = realloc(resp, sizeof(wchar_t) * (++resp_len + 1));
			memmove(resp + csr + 1, resp + csr, sizeof(wchar_t) * (resp_len - csr));
			resp[csr++] = k;
		}

		// interactively render response.
		if (csr < draw_start)
			draw_start = csr;
		else if (csr - draw_start >= ws.ws_col - render_col - 1)
			draw_start = csr - ws.ws_col + render_col + 1;

		draw_fill(ws.ws_row - 1, render_col, 1, ws.ws_col - render_col, L' ', CONF_A_GNORM_FG, CONF_A_GNORM_BG);
		for (size_t i = 0; i < resp_len - draw_start && i < ws.ws_col - render_col; ++i)
			draw_put_wch(render_row, render_col + i, resp[draw_start + i]);
		draw_put_attr(render_row, render_col + csr - draw_start, CONF_A_GHIGH_FG, CONF_A_GHIGH_BG, 1);

		draw_refresh();
	}

	resp[resp_len] = 0;
	return resp;
}

int
prompt_yes_no(wchar_t const *msg, bool deflt)
{
	size_t full_msg_len = wcslen(msg) + 8;
	wchar_t *full_msg = malloc(sizeof(wchar_t) * full_msg_len);
	wchar_t const *deflt_mk = deflt ? L"(Y/n)" : L"(y/N)";
	swprintf(full_msg, full_msg_len, L"%ls %ls ", msg, deflt_mk);
	
ask_again:;
	wchar_t *resp = prompt_ask(full_msg, NULL);
	if (!resp) {
		free(full_msg);
		return -1;
	}
	
	if (!wcscmp(resp, L"y") || deflt && !*resp) {
		free(resp);
		free(full_msg);
		return 1;
	} else if (!wcscmp(resp, L"n") || !deflt && !*resp) {
		free(resp);
		free(full_msg);
		return 0;
	} else {
		free(resp);
		prompt_show(L"expected either 'y' or 'n'!");
		goto ask_again;
	}
}

void
prompt_comp_path(wchar_t **resp, size_t *rlen, size_t *csr)
{
	if (*csr != *rlen)
		return;
	
	// 16 bytes added to path to avoid buffer overflow when, e.g. a 3 byte
	// character is written and `path_bytes` is `PATH_MAX` - 1.
	char path[PATH_MAX + 16] = {0};
	
	size_t path_bytes = 0, path_len = 0;
	while (path_len < *rlen && path_bytes < PATH_MAX) {
		int nbytes = wctomb(&path[path_bytes], (*resp)[path_len]);
		if (nbytes == -1)
			return;
		
		++path_len;
		path_bytes += nbytes;
	}
	
	if (path_bytes > PATH_MAX)
		return;
	
	char dir[PATH_MAX];
	strncpy(dir, path, PATH_MAX);
	char *first_slash = strchr(dir, '/'), *last_slash = strrchr(dir, '/');
	
	if (!last_slash) {
		dir[0] = '.';
		dir[1] = 0;
	} else if (first_slash && first_slash == last_slash)
		*(last_slash + 1) = 0;
	else
		*last_slash = 0;
	
	size_t dir_len = strlen(dir);
	
	char const *name = last_slash ? last_slash + 1 : path;
	size_t name_len = strlen(name);
	
	DIR *dir_p = opendir(dir);
	if (!dir_p)
		return;
	
	struct dirent *dir_ent;
	while (dir_ent = readdir(dir_p)) {
		if (strncmp_case_insen(name, dir_ent->d_name, name_len)
		    || dir_ent->d_type != DT_DIR && dir_ent->d_type != DT_REG
		    || !strcmp(".", dir_ent->d_name)
		    || !strcmp("..", dir_ent->d_name)) {
			continue;
		}
		
		char new_path[PATH_MAX];
		strcpy(new_path, dir);
		
		// only add '/' if not root, otherwise you'd get "//...".
		if (!first_slash || first_slash != last_slash)
			strcat(new_path, "/");
		
		strcat(new_path, dir_ent->d_name);
		if (dir_ent->d_type == DT_DIR)
			strcat(new_path, "/");
		size_t new_path_bytes = strlen(new_path);
		
		// temporarily allocate potentially too much memory, then reduce
		// when done.
		// this allows one pass to be done when converting to wide chars
		// rather than two, saving computation.
		*resp = realloc(*resp, sizeof(wchar_t) * (PATH_MAX + 1));
		
		// this assumes path name will be valid when read as multibyte
		// string in locale.
		size_t new_path_len = 0, new_path_bytes_read = 0;
		while (new_path_bytes_read < new_path_bytes) {
			wchar_t wch;
			size_t nbytes = mbstowcs(&wch, &new_path[new_path_bytes_read], 1);
			if (nbytes == (size_t)-1) {
				closedir(dir_p);
				return;
			}
			
			(*resp)[new_path_len++] = wch;
			new_path_bytes_read += nbytes;
		}
		
		*csr = *rlen = new_path_len;
		*resp = realloc(*resp, sizeof(wchar_t) * (new_path_len + 1));
		
		closedir(dir_p);
		
		return;
	}
	
	closedir(dir_p);
}

static void
draw_box(wchar_t const *text)
{
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);

	size_t text_rows = 1, line_width = 0;
	for (wchar_t const *c = text; *c; ++c) {
		if (++line_width >= ws.ws_col || *c == L'\n') {
			line_width = 0;
			++text_rows;
		}
	}

	size_t box_top = ws.ws_row - text_rows;
	draw_fill(box_top, 0, text_rows, ws.ws_col, L' ', CONF_A_GNORM_FG, CONF_A_GNORM_BG);
	draw_put_wstr(box_top, 0, text);
}
