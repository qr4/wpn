#ifndef ENTITIES_H
#define ENTITIES_H

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

typedef struct entity_t entity_t;

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
	void *lua_stuff;
};


struct slot_data_t {
	slot_t *slot;
};

struct planet_data_t {
	slot_t *slot;

	entity_t *planet;
	entity_t *asteroid;
	unsigned int asteroids;

	// fill with usefull stuff
};

struct ship_data_t {
	slot_t *slot;
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

/*
 * mapped vector functions
 */


/*
 * Returns the distance between 2 points squared. 
 * This is faster than dist().
 */
static inline double quaddist(const entity_t* A, const entity_t* B) {
	return vector_quaddist(&A->pos, &B->pos);
}

/*
 * Returns the distance between 2 points 
 */
static inline double dist(const entity_t* A, const entity_t* B) {
	return vector_dist(&A->pos, &B->pos);
}
/*
 * Decide on which side the point C is realtive to the line A - B is.
 *
 * = 0 -> C on the line A - B
 * > 0 -> C left of line A - B
 * < 0 -> C right of line A - B
 */
static inline double relative_position(const entity_t *A, const entity_t *B, const entity_t *C) {
	return vector_relative_position(&A->pos, &B->pos, &C->pos);
}


/*
 * Dividing ratio of C on A - B
 */
static inline double dividing_ratio(entity_t* A, entity_t* B, entity_t* C) {
	return vector_dividing_ratio(&A->pos, &B->pos, &C->pos);
}

/*
 * Minimal distance of C to the line A - B
 */
static inline double dist_to_line(entity_t* A, entity_t* B, entity_t* C) {
	return vector_dividing_ratio(&A->pos, &B->pos, &C->pos);
}

static inline double collision_dist(entity_t *A, entity_t *B) {
	return dist(A, B) - A->radius - B->radius;
}

/*
 * ship-functions
 */

static inline void move_ship(entity_t *ship) {
	vector_t dt = vector(0.05); //replace this by a global 
	ship->pos.v += ship->v.v * dt.v;
}

/*
 * Errorcodes for slot transfers
 */
typedef enum {
	OK = 0,
	OUT_OF_BOUNDS_LEFT,
	OUT_OF_BOUNDS_RIGHT,
	NO_SPACE,
} ETRANSFER;

/*
 * Swaps two slots. *left can be the same as *right
 */
ETRANSFER swap_slots(entity_t *left, int pos_left, entity_t *right, int pos_right);

/*
 * Moves one slot from left to right. 
 * Target position has to be empty.
 */
ETRANSFER transfer_slot(entity_t *left, int pos_left, entity_t *right, int pos_right);

void init_entity(entity_t *e, const vector_t pos, const type_t type, unsigned int slots);

#endif /* ENTITIES_H */
