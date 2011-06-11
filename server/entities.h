#ifndef ENTITIES_H
#define ENTITIES_H

#include <lua.h>
#include "vector.h"
#include "types.h"
#include "map.h"

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


void move_ship(entity_t *ship);

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
ETRANSFER swap_slots(entity_t *left, unsigned int pos_left, entity_t *right, unsigned int pos_right);

/*
 * Moves one slot from left to right.
 * Target position has to be empty.
 */
ETRANSFER transfer_slot(entity_t *left, unsigned int pos_left, entity_t *right, unsigned int pos_right);

/* Initialize the insides of an entity datastructure (this does not allocate the memory for the entity_t) */
void init_entity(entity_t *e, const vector_t pos, const type_t type, unsigned int slots);

/* Clean up an entity, to allow it to be subsequently freed */
void destroy_entity(entity_t *e);

/* Type-Bitfield to String converter for debugging and lulz
 * Don't free the return value! */
const char *type_string(type_t type);

/* Create a string representation of the contents of this entities'
 * slots. One byte per Slot. */
char* slots_to_string(entity_t* e);

#endif /* ENTITIES_H */
