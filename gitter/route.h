#ifndef ROUTE_H
#define ROUTE_H

#include "screen.h"

typedef struct wp_t waypoint_t;

struct wp_t {
        pixel_t point;
        waypoint_t* next;
};

waypoint_t *smooth(waypoint_t *way, int res);
waypoint_t* plotCourse(pixel_t* start, pixel_t* stop, pixel_t* points, int n);
waypoint_t* plotCourse_gridbased(pixel_t* start, pixel_t* stop, pixel_t* points, int n);

#endif  /*ROUTE_H*/
