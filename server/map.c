#include <stdlib.h>

#include "map.h"
#include "entities.h"

double AVERAGE_CLUSTER_DISTANCE = 500;
double MAXIMUM_CLUSTER_SIZE = 500 / 3;

double MINIMUM_PLANET_SIZE = 30;
double MAXIMUM_PLANET_SIZE = 50;
double PLANET_DENSITY = 0.25;

double ASTEROID_DENSITY = 0.60;
double AVERAGE_ASTEROID_NUMBER = 5;
double ASTEROID_VARIANCE;

double AVERAGE_GRID_SIZE = 500;

size_t CLUSTERS_X = 10;
size_t CLUSTERS_Y = 10;

void distribute_asteroids(entity_t *cluster, const unsigned int n);
void set_limits(map_t *map);

map_t *generate_map() {
	srand48(0);
	size_t x, y;
	vector_t pos;
	map_t *map;

	map = (map_t *) malloc(sizeof(map_t));
	map->clusters_x = CLUSTERS_X;
	map->clusters_y = CLUSTERS_Y;
	map->cluster = (entity_t *) malloc(sizeof(entity_t) * map->clusters_x * map->clusters_y);

	// position clusters at initial grid positions, try to avoid x,y values < 0
	for (y = 0, pos.y = AVERAGE_CLUSTER_DISTANCE; y < map->clusters_y ; y++, pos.y += AVERAGE_CLUSTER_DISTANCE) {
		for (x = 0, pos.x = AVERAGE_CLUSTER_DISTANCE; x < map->clusters_x; x++, pos.x += AVERAGE_CLUSTER_DISTANCE) {
			vector_t cluster_pos = pos;
			cluster_pos.v += randv().v * vector(AVERAGE_CLUSTER_DISTANCE / 3).v;
			init_entity(map->cluster + y * map->clusters_x + x, pos, CLUSTER, 0);
		}
	}
	
	// fill with planets / asteroids, set radius
	for (y = 0; y < map->clusters_y; y++) {
		for (x = 0; x < map->clusters_x; x++) {
			entity_t *working = &(map->cluster[map->clusters_x * y + x]);
			double r = drand48();

			if (r < PLANET_DENSITY) {
				working->cluster_data->planet = (entity_t *) malloc (sizeof(entity_t));
				working->cluster_data->asteroid = NULL;
				working->cluster_data->asteroids = 0;
				working->radius = PLANET_SIZE;
				init_entity(working->cluster_data->planet, working->pos, PLANET, 0);
			} else if (r < PLANET_DENSITY + ASTEROID_DENSITY) {
				distribute_asteroids(working, AVERAGE_ASTEROID_NUMBER);
			} else {
				// Empty cluster
				working->cluster_data->planet = NULL;
				working->cluster_data->asteroid = NULL;
				working->cluster_data->asteroids = 0;
		 	}
		}
	}
	
	// find area of the map
	set_limits(map);

	// build quads

	return map;
}

void distribute_asteroids(entity_t *cluster, const unsigned int n) {
	double max_distance = 0;
	double dist_to_cluster_center = 0;

	unsigned int i;
	unsigned int j;
	cluster->cluster_data->planet = NULL;
	cluster->cluster_data->asteroid = (entity_t *) malloc (sizeof(entity_t) * n);
	cluster->cluster_data->asteroids = n;
	// Distribute asteroids within cluster
	for (i = 0; i < n; i++) {
		entity_t *asteroid = &(cluster->cluster_data->asteroid[i]);

		asteroid->slots  = drand48() * 3 + 1;
		asteroid->radius = ASTEROID_RADIUS_TO_SLOTS_RATIO * asteroid->slots;

		asteroid->pos.v = cluster->pos.v + randv().v * vector(MAXIMUM_CLUSTER_SIZE).v;

		// Check for overlapping asteroids or too great distance to cluster-center
		for (j = 0; j < i;) {
			if (dist(asteroid, cluster) > MAXIMUM_CLUSTER_SIZE
					|| collision_dist(asteroid, &(cluster->cluster_data->asteroid[j])) < 0) {
				asteroid->pos.v = cluster->pos.v + randv().v * vector(MAXIMUM_CLUSTER_SIZE).v;
				j = 0;
			} else {
				j++;
			}
		}

		init_entity(asteroid, asteroid->pos, ASTEROID, asteroid->slots);

		// find maximum distance to cluster-center
		dist_to_cluster_center = dist(cluster, asteroid) + asteroid->radius;
		if (dist_to_cluster_center > max_distance) {
			max_distance = dist_to_cluster_center;
		}
	}

	// set safety-radius as maximum distance of a asteroid to cluster-center
	cluster->radius = max_distance;
}

void set_limits(map_t *map) {
	int i;
	double left_bound, upper_bound, right_bound, lower_bound;

	// find upper bound

	upper_bound = map->cluster[0].pos.y - map->cluster[0].radius;
	lower_bound = map->cluster[0].pos.y + map->cluster[0].radius;
	left_bound  = map->cluster[0].pos.x - map->cluster[0].radius;
	right_bound = map->cluster[0].pos.x + map->cluster[0].radius;

	// find upper bound
	for (i = 1; i < map->clusters_x; i++) {
		double t = map->cluster[i].pos.y - map->cluster[i].radius;
		if (t < upper_bound) {
			upper_bound = t;
		}
	}

	// find lower bound
	for (i = (map->clusters_y - 1) * map->clusters_x; i < map->clusters_x * map->clusters_y; i++) {
		double t = map->cluster[i].pos.y + map->cluster[i].radius;
		if (t > upper_bound) {
			lower_bound = t;
		}
	}
	
	for (i = 0; i < map->clusters_x * map->clusters_y; i += map->clusters_x) {
		double t = map->cluster[i].pos.x - map->cluster[i].radius;
		if (t < left_bound) {
			left_bound = t;
		}

		t = map->cluster[i + map->clusters_x - 1].pos.x - map->cluster[i + map->clusters_x - 1].radius;
		if (t > left_bound) {
			right_bound = t;
		}
	}

	map->left_bound = left_bound;
	map->upper_bound = upper_bound;
	map->right_bound = right_bound;
	map->lower_bound = lower_bound;
}
