#ifndef TYPES_H
#define TYPES_H

#include <lua.h>
#include "vector.h"

typedef enum {
	EMPTY,
	WEAPON,
	DRIVE,
	ORE,
	BLOCKED,   // signals, that this slot may not be altered
	SHIELD,    // currently unused
} slot_t;

typedef enum {
	CLUSTER,
	PLANET,
	ASTEROID,
	BASE,
	SHIP,
} type_t;

typedef struct slot_data_t     slot_data_t;
typedef struct planet_data_t   planet_data_t;
typedef struct ship_data_t     ship_data_t;
typedef struct asteroid_data_t asteroid_data_t;
typedef struct cluster_data_t  cluster_data_t;
typedef struct base_data_t     base_data_t;

typedef struct entity_t     entity_t;
typedef struct map_quad_t   map_quad_t;
typedef struct quad_index_t quad_index_t;
typedef struct map_t        map_t;

struct quad_index_t {
	size_t quad_x;
	size_t quad_y;
};

struct map_quad_t {
	entity_t **cluster;
	entity_t **static_object;
	entity_t **moving_object;
	size_t clusters;       // number of clusters in this quad
	size_t static_objects; // number of static objects (asteroids, planets)
	size_t moving_objects; // number of moving objects (ships)
};

struct map_t {
	entity_t *cluster;
	map_quad_t   *quad;
	size_t clusters_x;
	size_t clusters_y;
	size_t quad_size;
	size_t quads_x;
	size_t quads_y;
	double left_bound;
	double upper_bound;
	double right_bound;
	double lower_bound;
};

struct entity_t {
	vector_t pos;
	vector_t v;

	float radius;
	type_t type;
	unsigned int slots;
	unsigned int player_id;

	union {
		void            *data;
		slot_data_t     *slot_data;
		planet_data_t   *planet_data;
		ship_data_t     *ship_data;
		asteroid_data_t *asteroid_data;
		cluster_data_t  *cluster_data;
		base_data_t     *base_data;
	};
	lua_State* lua;
};

typedef struct wp_t waypoint_t;

typedef enum {
	WP_START,
	WP_TURN_START,
	WP_TURN_VIA,
	WP_TURN_STOP,
	WP_VIA,
	WP_STOP
} wptype;

struct wp_t {
	vector_t point;
	vector_t speed;
	double t;
	wptype type;
	vector_t obs;
	waypoint_t* next;
};

struct slot_data_t {
	slot_t *slot;
};

struct planet_data_t {
	slot_t *slot;
};

struct ship_data_t {
	slot_t *slot;
	waypoint_t *flightplan;
};

struct asteroid_data_t {
	slot_t *slot;
};

struct cluster_data_t {
	entity_t *planet;
	entity_t *asteroid;
	unsigned int asteroids;
};

struct base_data_t {
	slot_t *slot;
};

#endif  /*TYPES_H*/
