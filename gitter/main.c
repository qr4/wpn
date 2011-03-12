#include <stdio.h>
#include "screen.h"
#include "route.h"

int main(int argc, char *argv[]) {
	GLOBALS.WIDTH  = 768;
	GLOBALS.HEIGHT = 768;

	sdl_init();
	main_loop();

	return 0;
}
