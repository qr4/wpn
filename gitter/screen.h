#ifndef SCREEN_H
#define SCREEN_H

#include "globals.h"

typedef struct {
	float x;
	float y;
} vector_t;

typedef struct {
	vector_t center;
	float safety_radius;
} cluster_t;

typedef struct wp_t waypoint_t;

struct wp_t {
	vector_t point;
	waypoint_t* next;
};

void sdl_init();
int main_loop();

void draw_line(vector_t *p1, vector_t *p2);
void draw_blob(cluster_t* p);

#endif  /*SCREEN_H*/
