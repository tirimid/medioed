#include <ncurses.h>

int
main(void)
{
	initscr();
	raw();
	noecho();
	
	printw("hello world");
	refresh();
	getch();
	
	endwin();
	return 0;
}
