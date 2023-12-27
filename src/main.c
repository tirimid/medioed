#include <locale.h>
#include <stdbool.h>
#include <stdio.h>

#include <termios.h>
#include <unistd.h>

#include "draw.h"
#include "editor.h"
#include "util.h"

#define LOGFILE "medioed.log"
#define LOCALE "C.UTF-8"

static void usage(char const *name);

bool flag_c = false, flag_d = false, flag_r = false;

int
main(int argc, char const *argv[])
{	
	int ch;
	while ((ch = getopt(argc, (char *const *)argv, "cdhr")) != -1) {
		switch (ch) {
		case 'c':
			flag_c = true;
			break;
		case 'd':
			flag_d = true;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		case 'r':
			flag_r = true;
			break;
		default:
			return 1;
		}
	}
	
	if (flag_c && flag_r) {
		fputs("cannot specify -c and -r at the same time!\n", stderr);
		return 1;
	}

	FILE *logfp = fopen(flag_d ? LOGFILE : "/dev/null", "w");
	if (!logfp) {
		fputs("cannot open logfile: " LOGFILE "!\n", stderr);
		return 1;
	}
	stderr = logfp;
	
	if (!setlocale(LC_ALL, LOCALE)) {
		fputs("failed on setlocale() to " LOCALE "!\n", stderr);
		fclose(logfp);
		return 1;
	}
	
	struct termios old;
	if (tcgetattr(STDIN_FILENO, &old)) {
		fputs("failed on tcgetattr() for stdin!\n", stderr);
		fclose(logfp);
		return 1;
	}

	struct termios new = old;
	new.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	new.c_iflag &= ~(ICRNL | IXON | ISTRIP);
	new.c_oflag &= ~OPOST;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new)) {
		fputs("failed on tcsetattr() making stdin raw!\n", stderr);
		fclose(logfp);
		return 1;
	}

	setvbuf(stdin, NULL, _IONBF, 0);
	
	draw_init();
	
	if (editor_init(argc, argv)) {
		fputs("failed on editor_init()!\n", stderr);
		fclose(logfp);
		return 1;
	}
	editor_mainloop();
	editor_quit();
	
	draw_quit();

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &old)) {
		fputs("failed on tcsetattr() restoring old stdin!\n", stderr);
		fclose(logfp);
		return 1;
	}

	fclose(logfp);
	
	return 0;
}

static void
usage(char const *name)
{
	printf("usage:\n"
	       "\t%s [options] file ...\n"
	       "\t%s [options]\n"
	       "options:\n"
	       "\t-c  create opened file if it doesn't exist\n"
	       "\t-d  try to write debug output to a logfile\n"
	       "\t-h  display this menu\n"
	       "\t-r  existing files will be opened read-only\n", name, name);
}
