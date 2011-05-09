#include <stdio.h>
#include <string.h>

#include "entities.h"

/*
 * Swaps two slots. *left can be the same as *right
 */
ETRANSFER swap_slots(entity_t *left, int pos_left, entity_t *right, int pos_right) {
	if (left->slots < pos_left + 1 || pos_left < 0) {
		return OUT_OF_BOUNDS_LEFT;
	}
	if (right->slots < pos_right + 1 || pos_right < 0) {
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
ETRANSFER transfer_slot(entity_t *left, int pos_left, entity_t *right, int pos_right) {
	if (left->slots < pos_left + 1 || pos_left < 0) {
		return OUT_OF_BOUNDS_LEFT;
	}
	if (right->slots < pos_right + 1 || pos_right < 0) {
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
	slots = (slots > 255) ? 255 : slots;
	e->type  = type;
	e->slots = slots;
	
	switch (type) {
		case CLUSTER :
			e->cluster_data  = (cluster_data_t *)  malloc (sizeof(cluster_data_t));
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
			e->v = vector(0);
			break;
		default :
			fprintf(stderr, "%s in %s: Unknown type %d!", __func__, __FILE__, type);

			if (slots > 0) {
				e->slot_data = (slot_data_t *) malloc (sizeof(slot_data_t));
			}
	}

	if (slots > 0) {
		int i;

		e->slot_data->slot = malloc(sizeof(slot_t) * slots);

		for (i = 0; i < slots; i++) {
			e->slot_data->slot[i] = EMPTY;
		}
	}
}

void destroy_entity(entity_t **e) {
	type_t type = (*e)->type;
	switch (type) {
		case CLUSTER :
			break;
		case PLANET :
			break;
		case ASTEROID :
			break;
		case BASE :
			break;
		case SHIP :
			break;
		default :
			break;
	}
}
