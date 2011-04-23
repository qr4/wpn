#include <stdlib.h>

#include "map.h"
#include "entities.h"

double AVERAGE_CLUSTER_DISTANCE;
double PLANET_DENSITY;
double ASTEROID_DENSITY;
double AVERAGE_ASTEROID_NUMBER;
double ASTEROID_VARIANCE;

size_t CLUSTERS_X;
size_t CLUSTERS_Y;

inline double randd() {
	return ((double) rand()) / ((double) RAND_MAX);
}

map_t *generate_map() {
	size_t x, y;
	vector_t pos;
	map_t *map;

	map = (map_t *) malloc(sizeof(map_t));
	map->clusters_x = CLUSTERS_X;
	map->clusters_y = CLUSTERS_Y;
	map->cluster = (entity_t *) malloc(sizeof(entity_t) * map->clusters_x * map->clusters_y);

	// position clusters at initial grid positions
	for (y = 0, pos.y = 0; y < map->clusters_y ; y++, pos.y += AVERAGE_CLUSTER_DISTANCE) {
		for (x = 0, pos.x = 0; x < map->clusters_x; x++, pos.x += AVERAGE_CLUSTER_DISTANCE) {
			init_entity(map->cluster + y * map->clusters_x + x, pos, CLUSTER, 0);
		}
	}
	
	// fill with planets / asteroids, set radius
	for (y = 1; y < map->clusters_y - 1 ; y++) {
		for (x = 1; x < map->clusters_x - 1; x++) {
			entity_t *working = map->cluster + map->clusters_x * y + x;
			double r = randd();

			if (r < PLANET_DENSITY) {
				working->cluster_data->planet = (entity_t *) malloc (sizeof(entity_t));
				working->cluster_data->asteroid = NULL;
				working->cluster_data->asteroids = 0;
			//	init_entity(working->static_object, working->pos, PLANET, 0);
			} else if (r < PLANET_DENSITY + ASTEROID_DENSITY) {
				// Cluster will have asteroids
				unsigned int n = 4;
				working->cluster_data->planet = NULL;
				working->cluster_data->asteroid = (entity_t *) malloc (sizeof(entity_t) * n);
				working->cluster_data->asteroids = n;
			}
		}
	}
	
	// build quads

	return map;
}
