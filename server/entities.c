#include <stdio.h>
#include <string.h>

#include "types.h"
#include "entities.h"
#include "map.h"
#include "physics.h"

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
	e->pos.v = pos.v;
	slots = (slots > 255u) ? 255u : slots;
	e->type  = type;
	e->slots = slots;

	switch (type) {
		case CLUSTER :
			// init as empty cluster
			e->cluster_data  = (cluster_data_t *)  malloc (sizeof(cluster_data_t));
			e->cluster_data->planet = NULL;
			e->cluster_data->asteroid = NULL;
			e->cluster_data->asteroids = 0;
			e->radius = 0;
			break;
		case PLANET :
			e->planet_data   = (planet_data_t *)   malloc (sizeof(planet_data_t));
			e->radius = PLANET_SIZE;
			break;
		case ASTEROID :
			e->asteroid_data = (asteroid_data_t *) malloc (sizeof(asteroid_data_t));
			e->radius = ASTEROID_RADIUS_TO_SLOTS_RATIO * slots;
			break;
		case BASE :
			e->base_data     = (base_data_t *)     malloc (sizeof(base_data_t));
			break;
		case SHIP :
			e->ship_data     = (ship_data_t *)     malloc (sizeof(ship_data_t));
			e->ship_data->flightplan = NULL;
			e->v = vector(0);
			break;
		default :
			fprintf(stderr, "%s in %s: Unknown type %d!", __func__, __FILE__, type);

			if (slots > 0) {
				e->slot_data = (slot_data_t *) malloc (sizeof(slot_data_t));
			}
	}

	if (slots > 0) {
		unsigned int i;

		e->slot_data->slot = malloc(sizeof(slot_t) * slots);

		for (i = 0; i < slots; i++) {
			e->slot_data->slot[i] = EMPTY;
		}
	}
}

void destroy_entity(entity_t *e) {
	unsigned int i;
	type_t type;

	if (e == NULL) return;

	if (e->slots > 0) {
		free(e->slot_data->slot);
		e->slot_data->slot = NULL;
	}

	type = (e)->type;

	// uncomment if costum free are nescessary
	switch (type) {
		case CLUSTER :
			destroy_entity(e->cluster_data->planet);
			free(e->cluster_data->planet);
			e->cluster_data->planet = NULL;

			for (i = 0; i < e->cluster_data->asteroids; i++) {
				destroy_entity(e->cluster_data->asteroid + i);
			}

			free(e->cluster_data->asteroid);
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
			free((e)->data);
			e->data = NULL;
			break;
		default :
			break;
	}
}
