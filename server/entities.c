#include <stdio.h>
#include <string.h>

#include "types.h"
#include "entities.h"
#include "map.h"
#include "physics.h"
#include "luastate.h"
#include "debug.h"
#include "entity_storage.h"
#include "storages.h"

extern double asteroid_radius_to_slots_ratio;
extern double planet_size;

/*
 * Swaps two slots. *left can be the same as *right
 */
ETRANSFER swap_slots(entity_t *left, unsigned int pos_left, entity_t *right, unsigned int pos_right) {
	if (left->slots <= pos_left) {
		return OUT_OF_BOUNDS_LEFT;
	}
	if (right->slots <= pos_right) {
		return OUT_OF_BOUNDS_RIGHT;
	}

	slot_t *slot_left  = left->slot_data->slot  + pos_left;
	slot_t *slot_right = right->slot_data->slot + pos_right;
	slot_t t;

	t = *slot_left;
	*slot_left = *slot_left;
	*slot_right = t;

	return OK;
}

/*
 * Moves one slot from left to right.
 * Target position has to be empty.
 */
ETRANSFER transfer_slot(entity_t *left, unsigned int pos_left, entity_t *right, unsigned int pos_right) {
	if (left->slots <= pos_left) {
		return OUT_OF_BOUNDS_LEFT;
	}
	if (right->slots <= pos_right) {
		return OUT_OF_BOUNDS_RIGHT;
	}

	slot_t *slot_left  = left->slot_data->slot  + pos_left;
	slot_t *slot_right = right->slot_data->slot + pos_right;

	if (*slot_right != EMPTY) {
		return NO_SPACE;
	}

	*slot_right = *slot_left;
	*slot_left = EMPTY;

	return OK;
}

void init_entity(entity_t *e, const vector_t pos, const type_t type, unsigned int slots) {
	e->pos = pos;
	e->v   = vector(0);
	slots = (slots > 255u) ? 255u : slots;
	e->type  = type;
	e->slots = slots;
	e->data  = NULL;

	e->radius = 1;
	e->player_id = 0;

	switch (type) {
		case CLUSTER :
			// init as empty cluster
			e->cluster_data  = (cluster_data_t *)  malloc (sizeof(cluster_data_t));
			e->cluster_data->planet   = INVALID_ID;
			e->cluster_data->asteroid = NULL;
			e->cluster_data->asteroids = 0;
			e->radius = 0;
			break;
		case PLANET :
			e->planet_data   = (planet_data_t *)   malloc (sizeof(planet_data_t));
			e->radius = planet_size;
			break;
		case ASTEROID :
			e->asteroid_data = (asteroid_data_t *) malloc (sizeof(asteroid_data_t));
			e->radius = asteroid_radius_to_slots_ratio * slots;
			e->asteroid_data->docked_to = INVALID_ID;
			break;
		case BASE :
			e->base_data     = (base_data_t *)     malloc (sizeof(base_data_t));
			e->base_data->timer_value = 0;
			e->base_data->docked_to = INVALID_ID;
			break;
		case SHIP :
			e->ship_data     = (ship_data_t *)     malloc (sizeof(ship_data_t));
			e->ship_data->flightplan = NULL;
			e->ship_data->timer_value = 0;
			e->ship_data->docked_to = INVALID_ID;
			break;
		default :
			ERROR("%s in %s: Unknown type %d!", __func__, __FILE__, type);

			if (slots > 0) {
				e->slot_data = (slot_data_t *) malloc (sizeof(slot_data_t));
			}
			break;
	}

	if (slots > 0) {
		unsigned int i;

		e->slot_data->slot = malloc(sizeof(slot_t) * slots);

		for (i = 0; i < slots; i++) {
			e->slot_data->slot[i] = EMPTY;
		}
	}

	e->lua=NULL;
}

void destroy_entity(entity_t *e) {
	unsigned int i;
	type_t type;

	if (e == NULL) return;

	if (e->slot_data != NULL && e->slots > 0) {
		free(e->slot_data->slot);
		e->slot_data->slot = NULL;
		e->slots = 0;
	}

	type = e->type;

	if(e->lua != NULL) {
		lua_close(e->lua);
	}

	switch (type) {
		case CLUSTER :
			free_entity(planet_storage, e->cluster_data->planet);

			for (i = 0; i < e->cluster_data->asteroids; i++) {
				free_entity(asteroid_storage, e->cluster_data->asteroid[i]);
			}
			free(e->cluster_data->asteroid);

			e->cluster_data->planet   = INVALID_ID;
			e->cluster_data->asteroid = NULL;
			e->cluster_data->asteroids = 0;

			break;
		case PLANET :
			break;
		case ASTEROID :
			break;
		case BASE :
			break;
		case SHIP :
			free_route(e->ship_data->flightplan);
			break;
		default :
			break;
	}

	// Free data pointer and unregister from grid
	switch (type) {
		case CLUSTER :
		case PLANET :
		case ASTEROID :
		case BASE :
		case SHIP :
			unregister_object(e);
			free(e->data);
			e->data = NULL;
			break;
		default :
			break;
	}
}

/* Type-Bitfield to String converter for debugging and lulz
 * Don't free the return value! */
const char *type_string(type_t type) {
	switch (type) {
		case CLUSTER :
			return "cluster";
			break;
		case BASE :
			return "base";
			break;
		case PLANET :
			return "planet";
			break;
		case ASTEROID :
			return "asteroid";
			break;
		case SHIP :
			return "ship";
			break;
		default :
			return "";
			break;
	}
}

/* Create a string representation of the contents of this entities'
 * slots. One byte per Slot. */
char* slots_to_string(entity_t* e) {
	int i;
	const char slot_mapping[] = " LTRXS";
	/*
	 * " ": EMPTY
	 * "L": LASER  / WEAPON
	 * "T": THRUST / DRIVE
	 * "R": ROCK   / ORE
	 * "X": UNUSEABLE / BLOCKED
	 * "S": SHIELD
	 */

	size_t slots = (e == NULL) ? 0 : e->slots;
	char* s;

	s = malloc(sizeof(char) * (slots + 1));

	if(!s) {
		ERROR("Malloc failed in slots_to_string\n");
		exit(1);
	}

	for(i = 0; i < slots; i++) {
		s[i] = slot_mapping[e->slot_data->slot[i]];
	}

	s[i] = '\0';

	return s;
}

void move_ship(entity_t *ship) {
	if(ship->ship_data != NULL) {
		if(ship->ship_data->flightplan != NULL) {
			waypoint_t* next = ship->ship_data->flightplan->next;
			free_waypoint(ship->ship_data->flightplan);
			ship->ship_data->flightplan = next;
			if(next) {
				ship->pos = next->point;
				ship->v = next->speed;
			} else {
				// *Bling* we arrived
				call_entity_callback(ship, AUTOPILOT_ARRIVED);
			}
		}
	}
}
