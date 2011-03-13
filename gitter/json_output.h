#ifndef JSON_H
#define JSON_H

#include "screen.h"
#include "route.h"

void json_output(pixel_t* planets, int n_planets);
void json_update(waypoint_t* route1, waypoint_t* route2i, int n_planets);

#endif  /*JSON_H*/
