#ifndef GLOBALS_H
#define GLOBALS_H

typedef struct {
	int WIDTH;
	int HEIGHT;
	int DISPLAY_WIDTH;
	int DISPLAY_HEIGHT;
	int FULLSCREEN_WIDTH;
	int FULLSCREEN_HEIGHT;
	int DELTA;
	int FULLSCREEN;
	int VERBOSE;
} options_t;

options_t GLOBALS;

#define VERBOSE(...) \
	if (GLOBALS.VERBOSE) {\
		printf(__VA_ARGS__);\
	}

#define ERROR(...) fprintf(stderr, __VA_ARGS__);
#endif  /*GLOBALS_H*/
