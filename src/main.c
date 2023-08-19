#include "editor.h"

int
main(int argc, char const *argv[])
{
	editor_init(argc, argv);
	editor_main_loop();
	editor_quit();
	
	return 0;
}
