#ifndef ROUTE_H
#define ROUTE_H

int autopilot_planner(entity_t* e, double x, double y);
int moveto_planner(entity_t* e, double x, double y);
int stop_planner(entity_t* e);

#endif

