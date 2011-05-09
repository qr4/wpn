#ifndef MAP_H
#define MAP_H

#include "entities.h"
#include "vector.h"

typedef struct {
	entity_t **cluster;
	entity_t **static_object;
	entity_t **moving_object;
	size_t clusters;       // number of clusters in this quad
	size_t static_objects; // number of static objects (asteroids, planets)
	size_t moving_objects; // number of moving objects (ships)
} map_quad_t;

typedef struct {
	entity_t *cluster;
	map_quad_t   *quad;
	size_t clusters_x;
	size_t clusters_y;
	size_t quad_size;
	double left_bound;
	double upper_bound;
	double right_bound;
	double lower_bound;
} map_t;


map_t *generate_map();
#endif  /*MAP_H*/
