#include <stdio.h>

#include "editor.h"

int
main(int argc, char const *argv[])
{
	if (editor_init(argc, argv)) {
		fputs("critical initialization error!\n", stderr);
		return 1;
	}
	
	editor_main_loop();
	editor_quit();
	
	return 0;
}
