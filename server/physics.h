#ifndef PHYSICS_H
#define PHYSICS_H
#include "types.h"
waypoint_t* create_waypoint(double x, double y, double vx, double vy, double t, wptype type);
void free_waypoint(waypoint_t* wp);
void free_route(waypoint_t* head);
double get_acceleration(entity_t* s);
void complete_flightplan(entity_t* s);
#endif
