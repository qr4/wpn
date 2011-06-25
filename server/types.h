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
	AUTOPILOT_ARRIVED,    // The autopilot has finished it's flight plan
	ENTITY_APPROACHING,   // Another entity has appeared within sensor radius
	ENTITY_IN_RANGE,      // Another entity is in weapons range
	SHOT_AT,              // You are being shot at
	WEAPONS_READY,        // After firing, the weapons have recharged and are back on-line
	BEING_DOCKED,         // You are being docked
	DOCKING_COMPLETE,     // The docking operation you requested has completed
	UNDOCKING_COMPLETE,   // Releasing the docking clamps has completed
	BEING_UNDOCKED,       // The other ship is undocking us.
	TRANSFER_COMPLETE,    // Transfer of a slot has completed.
	BUILD_COMPLETE,       // Production of a new ship completed (for bases)
	TIMER_EXPIRED,        // A fixed timer has expired
	MINING_COMPLETE,      // Digging pays off afterall
	MANUFACTURE_COMPLETE, // Congratulations, you built something
	COLONIZE_COMPLETE,  // A new planet has been colonized.
	UPGRADE_COMPLETE,  // A base finished doubling its own size
	HOMEBASE_KILLED,       // The player's homebase has been killed.

	NUM_EVENTS
} event_t;

/*
 * Type specific data type for entity_t.
 */
typedef struct slot_data_t     slot_data_t;
typedef struct planet_data_t   planet_data_t;
typedef struct ship_data_t     ship_data_t;
typedef struct asteroid_data_t asteroid_data_t;
typedef struct cluster_data_t  cluster_data_t;
typedef struct base_data_t     base_data_t;

/*
 * Main structures
 */
typedef struct entity_t     entity_t;     // almost everything is a entity
typedef struct map_quad_t   map_quad_t;   // entities are registered in a grid
typedef struct quad_index_t quad_index_t; // location of a quad within the grid
typedef struct map_t        map_t;        // main map structure

/* 
 * abstract view on entities, necessary to ensure uniquness
 * of every entity ever created.
 */
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
	entity_id_t *cluster;       // clusters are stored here
	entity_id_t *static_object; // static objects (bases, asteroids, planets) are stored here
	entity_id_t *moving_object; // ships are stored here
	size_t clusters;       // number of clusters in this quad
	size_t static_objects; // number of static objects (asteroids, planets, bases)
	size_t moving_objects; // number of moving objects (ships)
};

struct map_t {
	map_quad_t   *quad; // grid is stored here
	size_t quad_size;   // information about grid resolution
	size_t quads_x;     
	size_t quads_y;
	// map size
	double left_bound;
	double upper_bound;
	double right_bound;
	double lower_bound;
};

// Macro defining an always invalid ID
#define INVALID_ID ((entity_id_t) {.id = ~(0l)})

/* 
 * The entity_t datastructure holds all of the actual entity information.
 * Most entities are held in storages, that assign an unique id and
 * do the memory management. (see the storages.[hc] and entity_storage.[hc]
 *
 * Also many functions on entities like distance are defined.
 */
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

// all entities having slots can access this structure
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

	/* Possible context for this event (who have we just docked? What was I
	 * shooting again?) */
	entity_id_t timer_context;

	waypoint_t *flightplan;

	/* store the last quad this ship was in. needed by unregister object*/
	map_quad_t *quad;

};

struct asteroid_data_t {
	slot_t *slot;

	/* If docked: ID of the docking partner */
	entity_id_t docked_to;

	entity_t* cluster;
};

struct cluster_data_t {
	entity_id_t    planet; // only one Planet can be in a cluster
	entity_id_t *asteroid; // list of all asteroids in cluster
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

	/* Possible context for this event (who have we just docked? What was I
	 * shooting again?) */
	entity_id_t timer_context;

	/* The planet I sit on. */
	entity_id_t my_planet;

};

/* Information belonging to a player */
typedef struct player_data_t {
	unsigned int player_id;
	char* name;
	entity_id_t homebase;
} player_data_t;


/*
 * structure used for pooling
 */
typedef struct entity_storage entity_storage_t;

struct entity_storage {
	entity_t* entities;
	uint32_t* offsets;
	uint32_t max;
	uint32_t first_free;
	uint16_t type;
};

#endif  /*TYPES_H*/
