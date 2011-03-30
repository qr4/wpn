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

typedef struct {
	vector_t pos;
	union {
		vector_t v;

		// if an object is static, the following 128 bit can be used freely:
		struct {
			void *data1; 
			void *data2;
		};

		// add more for different entities
	};
	float radius;
	struct {
		unsigned int slots:8;      // number of slots, zero means has no slots or a planet has no base
		unsigned int ship:1;       // tell if it is a ship
		unsigned int asteroid:1;   
		unsigned int planet:1;
		unsigned int base:1;
		unsigned int cluster:1;
	} type;
	void *data; // if the entity has slots, the first member of the structure pointed to by *data has to be the the array pointer where the slots are saved.
} entity_t;

typedef struct {
	slot_t *slot;
} basic_slot_t;

/*
 * test-functions
 */
static inline int num_of_slots(entity_t *e) {
	return e->type.slots;
}

static inline int has_slots(entity_t *e) {
	return e->type.slots > 0;
}

static inline int is_ship(entity_t *e) {
	return e->type.slots != 0;
}

static inline int is_planet(entity_t *e) {
	return e->type.planet != 0;
}


static inline int is_asteroid(entity_t *e) {
	return e->type.asteroid != 0;
}

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

#endif /* ENTITIES_H */
