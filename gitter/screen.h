#ifndef SCREEN_H
#define SCREEN_H

#include "globals.h"

typedef struct {
	float x;
	float y;
} pixel_t;

extern int safety_radius;

void sdl_init();
int main_loop();

void draw_line(pixel_t *p1, pixel_t *p2);
void draw_blob(pixel_t *p1);

#endif  /*SCREEN_H*/
