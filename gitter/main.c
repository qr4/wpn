#include <stdio.h>
#include "screen.h"
#include "route.h"

int main(int argc, char *argv[]) {
	GLOBALS.WIDTH  = 1024;
	GLOBALS.HEIGHT = 1024;

	sdl_init();
	main_loop();

	return 0;
}
