#ifndef MAP_H
#define MAP_H

#include "entities.h"
#include "vector.h"

typedef struct {
	entity_t *cluster[4];
	entity_t *static_object;
	size_t static_objects;
	entity_t *moving_object;
	size_t moving_objects;
} cluster_quad_t;

typedef struct {
	entity_t *cluster;
	quad_t   *quad;
	size_t clusters_x;
	size_t clusters_y;
	vector_t size;
} map_t;
#endif  /*MAP_H*/
