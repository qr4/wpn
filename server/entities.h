#ifndef ENTITIES_H
#define ENTITIES_H

#include "vector.h"

#define HAS_SLOTS (1 <<  8) // every object having slots has this bit set
#define SHIP      (1 <<  9) // every ship has this bit set
#define PLANET    (1 << 10) // every has this bit set
#define ASTEROID  (1 << 11) // every static object has this bit set

#define SHIP_WITH_SLOTS     (SHIP | HAS_SLOTS)
#define PLANET_WITH_SLOTS   (PLANET | HAS_SLOTS)
#define ASTEROID_WITH_SLOTS (ASTEROID | HAS_SLOTS)

#define SLOTS_XXS   1
#define SLOTS_XS    2
#define SLOTS_S     3
#define SLOTS_M     6
#define SLOTS_L    12
#define SLOTS_XL   24
#define SLOTS_XXL 255 // maximum

typedef enum {
	// artificial
	MAP     = (1 << 31),
	CLUSTER = (1 << 30),

	// Planet without base
	PLANET_EMPTY = PLANET,
	// Panets with bases of diefferent size
	PLANET_S     = PLANET_WITH_SLOTS + SLOTS_S,
	PLANET_M     = PLANET_WITH_SLOTS + SLOTS_M,
	PLANET_L     = PLANET_WITH_SLOTS + SLOTS_L,
	PLANET_XL    = PLANET_WITH_SLOTS + SLOTS_XL,
	PLANET_XXL   = PLANET_WITH_SLOTS + SLOTS_XXL,
	
	
	// worthless ball of dirt
	ASTEROID_EMPTY = ASTEROID,
	// asteroids of different size
	ASTEROID_S     = ASTEROID_WITH_SLOTS + SLOTS_S,
	ASTEROID_M     = ASTEROID_WITH_SLOTS + SLOTS_M,
	ASTEROID_L     = ASTEROID_WITH_SLOTS + SLOTS_L,
	ASTEROID_XL    = ASTEROID_WITH_SLOTS + SLOTS_XL,
	ASTEROID_XXL   = ASTEROID_WITH_SLOTS + SLOTS_XXL,
	

	// moving objects with diefferent number of slots
	SHIP_S   = SHIP_WITH_SLOTS + SLOTS_S,
	SHIP_M   = SHIP_WITH_SLOTS + SLOTS_M,
	SHIP_L   = SHIP_WITH_SLOTS + SLOTS_L,
	SHIP_XL  = SHIP_WITH_SLOTS + SLOTS_XL,
	SHIP_XXL = SHIP_WITH_SLOTS + SLOTS_XXL,
} type_t;

typedef enum {
	BLOCKED,   // signals, that this slot may not be altered
	EMPTY,
	WEAPON,
	DRIVE,
	ORE,
	SHIELD,    // currently unused
} slot_t;

typedef struct {
	vector_t v;   // do not move!
	float radius;
	type_t type;
	void  *data;
} entity_t;

/*
 * Use this macro to ensure, that the slot-array is always
 * at the same position and sizes are consistent.
 * Additional data can be passed by EXTRA, 
 */
#define data_t(TYPE, SIZE, EXTRA) \
typedef struct {\
	slot_t slot[SLOTS_ ## SIZE];\
	EXTRA\
} TYPE ## _ ## SIZE ## _t

data_t(SHIP, S, ); // creates SHIP_S_t with SLOT_S slots.
data_t(SHIP, M, );
data_t(SHIP, L, );
data_t(SHIP, XL, );
data_t(SHIP, XXL, );

data_t(PLANET, S, );
data_t(PLANET, M, );
data_t(PLANET, L, );
data_t(PLANET, XL, );
data_t(PLANET, XXL, );

data_t(ASTEROID, S, );
data_t(ASTEROID, M, );
data_t(ASTEROID, L, );
data_t(ASTEROID, XL, );
data_t(ASTEROID, XXL, );

#undef data_t

/********************
* TESTING FUNCTIONS *
********************/
inline int num_of_slots(entity_t *e) {
	return (e->type) & 15;
}

inline int has_slots(entity_t *e) {
	return (e->type & HAS_SLOTS) > 0;
}

inline int is_ship(entity_t *e) {
	return (e->type & SHIP) > 0;
}

inline int is_planet(entity_t *e) {
	return (e->type & PLANET) > 0;
}

inline int is_asteroid(entity_t *e) {
	return (e->type & ASTEROID) > 0;
}

/*****************
* INIT FUNCTIONS *
*****************/

// undicided yet
#endif  /*ENTITIES_H*/
