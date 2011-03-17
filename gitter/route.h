#ifndef ROUTE_H
#define ROUTE_H

#include "screen.h"

waypoint_t* smooth(waypoint_t *way, int res);
waypoint_t* plotCourse(vector_t* start, vector_t* stop, cluster_t* clusters, int n);
waypoint_t* plotCourse_scanline_gridbased(vector_t* start, vector_t* stop, cluster_t* clusters, int n);
waypoint_t* plotCourse_gridbased(vector_t* start, vector_t* stop, cluster_t* clusters, int n);

#endif  /*ROUTE_H*/
