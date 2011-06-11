#ifndef ROUTE_H
#define ROUTE_H

void autopilot_planner(entity_t* e, double x, double y, char* callback);
void moveto_planner(entity_t* e, double x, double y, char* callback);
void stop_planner(entity_t* e, char* callback);

#endif

