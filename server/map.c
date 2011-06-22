#include <stdlib.h>
#include <stdio.h>

#include "map.h"
#include "config.h"
#include "entity_storage.h"
#include "storages.h"
#include "debug.h"

extern double asteroid_radius_to_slots_ratio;

double MAXIMUM_CLUSTER_SIZE = 500;

double MINIMUM_PLANET_SIZE = 30;
double MAXIMUM_PLANET_SIZE = 50;
double PLANETS = 25;

double ASTEROIDS = 150;
double MINIMUM_ASTEROIDS = 5;
double MAXIMUM_ASTEROIDS = 5;

double AVERAGE_GRID_SIZE = 500;

double MAP_SIZE_X = 100000.;
double MAP_SIZE_Y = 100000.;

size_t CLUSTERS_X = 40;
size_t CLUSTERS_Y = 40;

// Forward declarations for init_map()
static int check_for_cluster_collisions(entity_t *cluster);
static int check_for_asteroid_collisions(entity_t *asteroid, entity_t *cluster);
static void init_cluster_with_planet(entity_t *cluster);
static void init_cluster_with_asteroids(entity_t *cluster);
static void move_cluster_to_pos(entity_t *cluster, vector_t pos);
static void build_quads();

map_t map;

/******************\
* global functions *
\******************/

/* initialize the map
 * helper_functions at the end of this file
 */
void init_map() {
	srand48(1);
	size_t i;
	vector_t pos;
	vector_t map_size = (vector_t) {{MAP_SIZE_X, MAP_SIZE_Y}};
	entity_t *cluster;

	map.quad_size = AVERAGE_GRID_SIZE;

	// distribute all planets first, we all want a place to live after all
	for (i = 0; i < PLANETS; i++) {
		cluster = get_entity_by_id(alloc_entity(cluster_storage));
		init_entity(cluster, vector(0), CLUSTER, 0);
		init_cluster_with_planet(cluster);
		do {
			pos.v = map_size.v * (vector(0.5).v + vector(0.4).v * randv().v);
			move_cluster_to_pos(cluster, pos);
		} while (check_for_cluster_collisions(cluster) == 1);
	}
	
	// distribute all asteroids
	for (i = 0; i < ASTEROIDS; i++) {
		cluster = get_entity_by_id(alloc_entity(cluster_storage));
		init_entity(cluster, vector(0), CLUSTER, 0);
		init_cluster_with_asteroids(cluster);
		do {
			pos.v = map_size.v * (vector(0.5).v + vector(0.4).v * randv().v);
			move_cluster_to_pos(cluster, pos);
		} while (check_for_cluster_collisions(cluster) == 1);
	}
	
	map.left_bound = 0;
	map.upper_bound = 0;
	map.right_bound = MAP_SIZE_X;
	map.lower_bound = MAP_SIZE_Y;

	// build quads
	build_quads(map);
}

/* 
 * once upon a time, this function was huge an powerful.
 * but years passed by and other mighty function took away
 * its power and use. so all what was left of it was a 
 * sad one-liner. 
 */
void free_map() {
	free(map.quad);
}

/* generalized function to register an entity into the grid */
map_quad_t *register_object(entity_t *e) {
	map_quad_t *quad = get_quad_by_pos(e->pos); // the quad we want it to register in
	entity_id_t **object_pointer;               // storage, where type(e) lies. {cluster, static, moving}
	size_t *objects;

	// pick the right storage for the entity
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
			/* update quad for ship, as a ship can
			 * move out of a quad and then unregister_object
			 * would try to remove the entity from a wrong quad
			 */
			e->ship_data->quad = quad; 
			break;
		default :
			return NULL;
			break;
	}

	// make / free space;
	*object_pointer = (entity_id_t *) realloc (*object_pointer, (*objects + 1) * sizeof(entity_id_t));
	(*object_pointer)[(*objects)++] = e->unique_id;

	return quad;
}


/* generalized function to unregister an entity from the grid 
 * principles are the same like register_object */
void unregister_object(entity_t *e) {
	map_quad_t *quad;
	entity_id_t **object_pointer;
	size_t *objects;
	size_t i;

	if (e->type == SHIP) {
		quad = e->ship_data->quad;
	} else {
		quad = get_quad_by_pos(e->pos);
	}
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

	if (i == *objects) {
		ERROR("entity is not in the quad it is supposed to.\n");
	}

	// move last to this position
	(*object_pointer)[i] = (*object_pointer)[*objects - 1];
	(*objects)--;

	*object_pointer = (entity_id_t *) realloc (*object_pointer, *objects * sizeof(entity_id_t));
}

/* unregister and reregister an entity (update its grid-entry) */
map_quad_t* update_quad_object(entity_t *e) {
	unregister_object(e);
	return register_object(e);
}

// helper for find_closest()
static void get_search_bounds(vector_t pos, double radius, quad_index_t *start, quad_index_t *end) {
	vector_t up_left;
	vector_t down_right;

	up_left.v = pos.v - vector(radius).v;
	down_right.v = pos.v + vector(radius).v;

	*start = get_quad_index_by_pos(up_left);
	*end = get_quad_index_by_pos(down_right);
}

/*
 * find the closest object in the universe to e within a radius.
 * filter is used to match only against certain types (ships, planets, etc)
 */
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

/********************\
* init_map() helpers *
\********************/

// check all clusters within the universe for collision
static int check_for_cluster_collisions(entity_t *cluster) {
	size_t i;

	for (i = 0; i < cluster_storage->first_free; i++) {
		// check if one ship can pass safely between 2 clusters
		if (cluster_storage->entities + i != cluster &&
				collision_dist(cluster_storage->entities + i, cluster) < 2 * config_get_double("max_ship_size")) {
			return 1;
		}
	}

	return 0;
}

// check all asteroids within a cluster for collision
static int check_for_asteroid_collisions(entity_t *asteroid, entity_t *cluster) {
	size_t i;
	entity_t *to_check;

	for (i = 0; i < cluster->cluster_data->asteroids; i++) {
		to_check = get_entity_by_id(cluster->cluster_data->asteroid[i]);
		if (to_check != asteroid &&
				collision_dist(to_check, asteroid) < 2 * config_get_double("max_ship_size")) {
			return 1;
		}
	}

	return 0;
}
	
// move a cluster and its whole content to a new position
// planets will given the cluster's position
// asteroids are moved relativly to it (this might cause some precision issues when often updated)
static void move_cluster_to_pos(entity_t *cluster, vector_t pos) {
	entity_t *planet, *asteroid;
	vector_t diff;
	diff.v = pos.v - cluster->pos.v;

	cluster->pos = pos;

	if ((planet = get_entity_by_id(cluster->cluster_data->planet)) != NULL) {
		planet->pos = pos;
	}

	for (int i = 0; i < cluster->cluster_data->asteroids; i++) {
		asteroid = get_entity_by_id(cluster->cluster_data->asteroid[i]);
		asteroid->pos.v += diff.v;
	}
}

// distributes asteroids within a cluster and sets the cluster radius to a safe value
static void init_cluster_with_asteroids(entity_t *cluster) {
	entity_t *asteroid;
	size_t i;
	size_t asteroids = MINIMUM_ASTEROIDS + (MAXIMUM_ASTEROIDS - MINIMUM_ASTEROIDS) * drand48();
	double asteroid_radius;
	double cluster_radius;

	cluster->cluster_data->asteroid = realloc(cluster->cluster_data->asteroid, asteroids * sizeof(entity_id_t));

	for (i = 0; i < asteroids; i++) {
		asteroid = get_entity_by_id(alloc_entity(asteroid_storage));
		cluster->cluster_data->asteroid[i] = asteroid->unique_id;
		init_entity(asteroid, vector(0), ASTEROID, 5);
		asteroid_radius = asteroid->radius;
		asteroid->asteroid_data->cluster = cluster;

		do {
			asteroid->pos.v = cluster->pos.v + rad_randv().v * vector(
					(double) MAXIMUM_CLUSTER_SIZE 
					- 2*config_get_double("max_ship_size") 
					- asteroid_radius).v;
		} while (check_for_asteroid_collisions(asteroid, cluster) == 1);

		// increase cluster-size
		cluster_radius = dist(asteroid, cluster) + 2*config_get_double("max_ship_size") + asteroid_radius;
		if (cluster_radius > cluster->radius) {
			cluster->radius = cluster_radius;
		}

		cluster->cluster_data->asteroids++;
	}
}

// sets a planet in the middle of a cluster and sets the cluster radius to a safe value
static void init_cluster_with_planet(entity_t *cluster) {
	cluster->cluster_data->planet = alloc_entity(planet_storage);
	init_entity(get_entity_by_id(cluster->cluster_data->planet), vector(0), PLANET, 0);
	entity_t* e = get_entity_by_id(cluster->cluster_data->planet);
	init_entity(get_entity_by_id(cluster->cluster_data->planet), cluster->pos, PLANET, 0);
	e->planet_data->cluster = cluster;
	e->radius = drand48() * (MAXIMUM_PLANET_SIZE - MINIMUM_PLANET_SIZE) + MINIMUM_PLANET_SIZE;
	cluster->radius = e->radius + config_get_double("weapon_range") + 2*config_get_double("max_ship_size");
}

// registers all static objects after map-generation in the grid
static void build_quads() {
	entity_t *cluster;
	size_t i;
	size_t j;
	size_t quads_x = map.right_bound / map.quad_size + 1; // +1 in order to have a little room left
	size_t quads_y = map.lower_bound / map.quad_size + 1;

	map.quads_x = quads_x;
	map.quads_y = quads_y;
	map.quad = (map_quad_t *) calloc (quads_x * quads_y, sizeof(map_quad_t)); // use calloc, or init the pointer with NULL

	for (i = 0, cluster = cluster_storage->entities; i < cluster_storage->first_free; i++, cluster++) {
		DEBUG("cluster %lu\n", i);
		register_object(cluster);

		if (cluster->cluster_data->planet.id != INVALID_ID.id) {
			DEBUG("\thas planet\n");
			register_object(get_entity_by_id(cluster->cluster_data->planet));
		}

		for (j = 0; j < cluster->cluster_data->asteroids; j++) {
			DEBUG("\tasteroid\n");
			register_object(get_entity_by_id(cluster->cluster_data->asteroid[j]));
		}
	}
}
