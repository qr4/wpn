#include <stdlib.h>
#include <stdio.h>

#include "map.h"
#include "entity_storage.h"
#include "storages.h"
#include "debug.h"

extern double asteroid_radius_to_slots_ratio;

double AVERAGE_CLUSTER_DISTANCE = 500;
double MAXIMUM_CLUSTER_SIZE = 500 / 4.1;

double MINIMUM_PLANET_SIZE = 30;
double MAXIMUM_PLANET_SIZE = 50;
double PLANET_DENSITY = 0.25;

double ASTEROID_DENSITY = 0.60;
double AVERAGE_ASTEROID_NUMBER = 5;
double ASTEROID_VARIANCE;

double AVERAGE_GRID_SIZE = 500;

size_t CLUSTERS_X = 40;
size_t CLUSTERS_Y = 40;

void distribute_asteroids(entity_t *cluster, const unsigned int n);
void set_limits();
void build_quads();

map_t map;

void init_map() {
	srand48(1);
	size_t x, y, i;
	vector_t pos;
	entity_t *working;

	map.clusters_x = CLUSTERS_X;
	map.clusters_y = CLUSTERS_Y;
	//map.cluster = (entity_t *) malloc(sizeof(entity_t) * map.clusters_x * map.clusters_y);
	map.quad_size = AVERAGE_GRID_SIZE;

	// position clusters at initial grid positions, try to avoid x,y values < 0
	for (y = 0, pos.y = AVERAGE_CLUSTER_DISTANCE; y < map.clusters_y ; y++, pos.y += AVERAGE_CLUSTER_DISTANCE) {
		for (x = 0, pos.x = AVERAGE_CLUSTER_DISTANCE; x < map.clusters_x; x++, pos.x += AVERAGE_CLUSTER_DISTANCE) {
			working = get_entity_by_id(alloc_entity(cluster_storage));
			vector_t cluster_pos = pos;
			cluster_pos.v += randv().v * vector(MAXIMUM_CLUSTER_SIZE).v;
			init_entity(working, cluster_pos, CLUSTER, 0);
		}
	}

	// fill with planets / asteroids, set radius
	for (i = 0; (working = get_entity_by_index(cluster_storage, i)) != NULL; i++) {
		double r = drand48();

		if (r < PLANET_DENSITY) {
			working->cluster_data->planet = alloc_entity(planet_storage);
			entity_t* e = get_entity_by_id(working->cluster_data->planet);
			init_entity(get_entity_by_id(working->cluster_data->planet), working->pos, PLANET, 0);
			working->radius = get_entity_by_id(working->cluster_data->planet)->radius;
			e->planet_data->cluster = working;
		} else if (r < PLANET_DENSITY + ASTEROID_DENSITY) {
			distribute_asteroids(working, AVERAGE_ASTEROID_NUMBER + rand()%4 - 2);
		} else {
			// Empty cluster
		}
	}

	map.cluster = cluster_storage->entities;

	// find area of the map
	set_limits();

	// build quads
	build_quads(map);
}

void distribute_asteroids(entity_t *cluster, const unsigned int n) {
	double max_distance = 0;
	double dist_to_cluster_center = 0;

	unsigned int i;
	unsigned int j;
	cluster->cluster_data->asteroid = (entity_id_t *) malloc (sizeof(entity_id_t) * n);
	cluster->cluster_data->asteroids = n;
	// Distribute asteroids within cluster
	for (i = 0; i < n; i++) {
		cluster->cluster_data->asteroid[i] = alloc_entity(asteroid_storage);
		entity_t *asteroid = get_entity_by_id(cluster->cluster_data->asteroid[i]);

		DEBUG("Got id: %lu {.index = %u, .reincarnation = %u, .type = %u}\n", 
				asteroid->unique_id.id,
				asteroid->unique_id.index,
				asteroid->unique_id.reincarnation,
				asteroid->unique_id.type);

		asteroid->slots  = drand48() * 3 + 1;
		asteroid->radius = asteroid_radius_to_slots_ratio * asteroid->slots;

		asteroid->pos.v = cluster->pos.v + randv().v * vector(MAXIMUM_CLUSTER_SIZE).v;

		// Check for overlapping asteroids or too great distance to cluster-center
		for (j = 0; j < i;) {
			if (dist(asteroid, cluster) > MAXIMUM_CLUSTER_SIZE
					|| collision_dist(asteroid, get_entity_by_id(cluster->cluster_data->asteroid[j])) < 0) {
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

void set_limits() {
	int i;
	double left_bound, upper_bound, right_bound, lower_bound;

	// find upper bound

	upper_bound = map.cluster[0].pos.y - map.cluster[0].radius;
	lower_bound = map.cluster[0].pos.y + map.cluster[0].radius;
	left_bound  = map.cluster[0].pos.x - map.cluster[0].radius;
	right_bound = map.cluster[0].pos.x + map.cluster[0].radius;

	// find upper bound
	for (i = 1; i < map.clusters_x; i++) {
		double t = map.cluster[i].pos.y - map.cluster[i].radius;
		if (t < upper_bound) {
			upper_bound = t;
		}
	}

	// find lower bound
	for (i = (map.clusters_y - 1) * map.clusters_x; i < map.clusters_x * map.clusters_y; i++) {
		double t = map.cluster[i].pos.y + map.cluster[i].radius;
		if (t > lower_bound) {
			lower_bound = t;
		}
	}

	// find left and right bound
	for (i = 0; i < map.clusters_x * map.clusters_y; i += map.clusters_x) {
		double t = map.cluster[i].pos.x - map.cluster[i].radius;
		if (t < left_bound) {
			left_bound = t;
		}

		t = map.cluster[i + map.clusters_x - 1].pos.x - map.cluster[i + map.clusters_x - 1].radius;
		if (t > right_bound) {
			right_bound = t;
		}
	}

	map.left_bound = left_bound;
	map.upper_bound = upper_bound;
	map.right_bound = right_bound;
	map.lower_bound = lower_bound;
}

void free_map() {
	free(map.quad);
}

map_quad_t *register_object(entity_t *e) {
	map_quad_t *quad = get_quad_by_pos(e->pos);
	entity_id_t **object_pointer;
	size_t *objects;

	switch (e->type) {
		case CLUSTER :
			object_pointer = &(quad->cluster);
			objects = &(quad->clusters);
			break;
			//quad->cluster = (entity_t **) realloc (quad->cluster, (quad->clusters + 1) * sizeof(entity_t *));
			//quad->cluster[quad->clusters++] = e;
			//return quad;
		case BASE :
		case PLANET :
		case ASTEROID :
			object_pointer = &(quad->static_object);
			objects = &(quad->static_objects);
			break;
		case SHIP :
			object_pointer = &(quad->moving_object);
			objects = &(quad->moving_objects);
			break;
		default :
			return NULL;
			break;
	}

	*object_pointer = (entity_id_t *) realloc (*object_pointer, (*objects + 1) * sizeof(entity_id_t));
	(*object_pointer)[(*objects)++] = e->unique_id;

	return quad;
}

map_quad_t* update_quad_object(entity_t *e) {
	unregister_object(e);
	return register_object(e);
}


void build_quads() {
	entity_t *cluster;
	size_t i;
	size_t j;
	size_t quads_x = map.right_bound / map.quad_size + 1; // +1 in order to have a little room left
	size_t quads_y = map.lower_bound / map.quad_size + 1;

	map.quads_x = quads_x;
	map.quads_y = quads_y;
	map.quad = (map_quad_t *) calloc (quads_x * quads_y, sizeof(map_quad_t)); // use calloc, or init the pointer with NULL

	for (i = 0, cluster = map.cluster; i < map.clusters_x * map.clusters_y; i++, cluster++) {
		register_object(cluster);

		if (cluster->cluster_data->planet.id != INVALID_ID.id) {
			register_object(get_entity_by_id(cluster->cluster_data->planet));
		}

		for (j = 0; j < cluster->cluster_data->asteroids; j++) {
			register_object(get_entity_by_id(cluster->cluster_data->asteroid[j]));
		}
	}
}

void unregister_object(entity_t *e) {
	map_quad_t *quad = get_quad_by_pos(e->pos);
	entity_id_t **object_pointer;
	size_t *objects;
	size_t i;

	switch (e->type) {
		case CLUSTER :
			object_pointer = &(quad->cluster);
			objects = &(quad->clusters);
			break;
		case BASE :
		case PLANET :
		case ASTEROID :
			object_pointer = &(quad->static_object);
			objects = &(quad->static_objects);
			break;
		case SHIP :
			object_pointer = &(quad->moving_object);
			objects = &(quad->moving_objects);
			break;
		default :
			return;
			break;
	}

	if (*objects == 0) return;

	// find position of entity to delete
	for (i = 0; i < *objects && (*object_pointer)[i].id != e->unique_id.id; i++);

	// move last to this position
	(*object_pointer)[i] = (*object_pointer)[*objects - 1];
	(*objects)--;

	*object_pointer = (entity_id_t *) realloc (*object_pointer, *objects * sizeof(entity_id_t));
}

void get_search_bounds(vector_t pos, double radius, quad_index_t *start, quad_index_t *end) {
	vector_t up_left;
	vector_t down_right;

	up_left.v = pos.v - vector(radius).v;
	down_right.v = pos.v + vector(radius).v;

	*start = get_quad_index_by_pos(up_left);
	*end = get_quad_index_by_pos(down_right);
}

entity_t *find_closest(entity_t *e, const double radius, const unsigned int filter) {
	quad_index_t start, end;
	quad_index_t index;

	size_t x, y;
	unsigned int i;
	double dist = radius;
	double t;

	entity_t *closest = NULL;

	get_search_bounds(e->pos, radius, &start, &end);
	index = get_quad_index_by_pos(e->pos);

//	ERROR(stderr, "entity address: %lx\n", e);
//	ERROR(stderr, "map size: %lu\n", map.quad_size);
//	ERROR(stderr, "entity pos: (%f, %f)\n", e->pos.x, e->pos.y);
//	ERROR(stderr, "search goes from (%lu, %lu) to (%lu, %lu) for (%lu, %lu)\n", start.quad_x, start.quad_y,
//			end.quad_x, end.quad_y,
//			index.quad_x, index.quad_y);

	for (y = start.quad_y; y <= end.quad_y; y++) {
		for (x = start.quad_x ; x <= end.quad_x; x++) {
			map_quad_t *quad = get_quad_by_index((quad_index_t) {x, y});

			if (filter & CLUSTER) {
				for (i = 0; i < quad->clusters; i++) {
					if (filter & quad->cluster[i].type 
							&& e->unique_id.id != quad->cluster[i].id
							&& (t = collision_dist(e, get_entity_by_id(quad->cluster[i]))) < dist) {
						dist = t;
						closest = get_entity_by_id(quad->cluster[i]);
					}
				}
			}

			if (filter & (ASTEROID | PLANET)) {
				for (i = 0; i < quad->static_objects; i++) {
					if (filter & quad->static_object[i].type 
							&& e->unique_id.id != quad->static_object[i].id
							&& (t = collision_dist(e, get_entity_by_id(quad->static_object[i]))) < dist) {
						dist = t;
						closest = get_entity_by_id(quad->static_object[i]);
					}
				}
			}

			if (filter & SHIP) {
				for (i = 0; i < quad->moving_objects; i++) {
					if (filter & quad->moving_object[i].type 
							&& e->unique_id.id != quad->moving_object[i].id
							&& (t = collision_dist(e, get_entity_by_id(quad->moving_object[i]))) < dist) {
						dist = t;
						closest = get_entity_by_id(quad->moving_object[i]);
					}
				}
			}
		}
	}

	return closest;
}
