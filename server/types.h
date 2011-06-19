#ifndef TYPES_H
#define TYPES_H

#include <lua.h>
#include <stdint.h>
#include "vector.h"

typedef enum {
	EMPTY = 0,
	WEAPON,
	DRIVE,
	ORE,
	BLOCKED,   // signals, that this slot may not be altered
	SHIELD,    // currently unused
} slot_t;

typedef enum {
	CLUSTER   = 1 << 0,
	PLANET    = 1 << 1,
	ASTEROID  = 1 << 2,
	BASE      = 1 << 3,
	SHIP      = 1 << 4,
} type_t;

/* Enum of possible events which can trigger callbacks in a ship */
/* (Corresponding lua function names are define in luastate.c) */
typedef enum {
	AUTOPILOT_ARRIVED,  // The autopilot has finished it's flight plan
	ENTITY_APPROACHING, // Another entity has appeared within sensor radius
	SHOT_AT,            // You are being shot at
	WEAPONS_READY,      // After firing, the weapons have recharged and are back on-line
	BEING_DOCKED,       // You are being docked
	DOCKING_COMPLETE,   // The docking operation you requested has completed
	UNDOCKING_COMPLETE, // Releasing the docking clamps has completed
	TRANSFER_COMPLETE,  // Transfer of a slot has completed.
	BUILD_COMPLETE,  // Production of a new ship completed (for bases)
	TIMER_EXPIRED,      // A fixed timer has expired

	NUM_EVENTS
} event_t;

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

typedef union  entity_id_t  entity_id_t;

/* Entity IDs are 64bit integers, of which the lower
 * 32 bits denote the array index, and the upper 32
 * denote the type and respawn count of this slot */
union entity_id_t {
	struct {
		uint32_t index;
		uint16_t reincarnation;
		uint16_t type;
	};
	uint64_t id;
};

struct quad_index_t {
	size_t quad_x;
	size_t quad_y;
};

struct map_quad_t {
	entity_id_t *cluster;
	entity_id_t *static_object;
	entity_id_t *moving_object;
	size_t clusters;       // number of clusters in this quad
	size_t static_objects; // number of static objects (asteroids, planets)
	size_t moving_objects; // number of moving objects (ships)
};

struct map_t {
	entity_t     *cluster;
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

#define INVALID_ID ((entity_id_t) {.id = ~(0l)})

/* The entity_t datastructure holds all of the actual entity information */
struct entity_t {

	/* Position and velocity */
	vector_t pos;
	vector_t v;

	float radius;
	type_t type;
	unsigned int slots;
	unsigned int player_id;

	/* Unique id of this entity */
	entity_id_t unique_id;

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
	double swingbydist;
	waypoint_t* next;
};

struct slot_data_t {
	slot_t *slot;
};

struct planet_data_t {
	slot_t *slot;
	entity_t* cluster;
};

struct ship_data_t {
	slot_t *slot;

	/* If docked: ID of the docking partner */
	entity_id_t docked_to;

	/* Number of ticks until this ships' lua state is re-woken */
	int timer_value;

	/* Event to trigger when the timer reaches zero */
	event_t timer_event;

	waypoint_t *flightplan;
};

struct asteroid_data_t {
	slot_t *slot;

	/* If docked: ID of the docking partner */
	entity_id_t docked_to;
};

struct cluster_data_t {
	entity_id_t    planet;
	entity_id_t *asteroid;
	unsigned int asteroids;
};

struct base_data_t {
	slot_t *slot;

	/* If docked: ID of the docking partner */
	/* TODO: Shouldn't bases allow multiple docking partners? */
	entity_id_t docked_to;

	/* Number of ticks until this bases lua state is re-woken */
	int timer_value;

	/* Event to trigger when the timer reaches zero */
	event_t timer_event;
};

/* Information belonging to a player */
typedef struct player_data_t {
	unsigned int player_id;
	char* name;
	entity_id_t homebase;
} player_data_t;

#endif  /*TYPES_H*/
