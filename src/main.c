#include "editor.h"

int
main(void)
{
	editor_init();
	editor_main_loop();
	editor_quit();
	
	return 0;
}
