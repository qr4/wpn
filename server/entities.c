#include "entities.h"

/*
 * Swaps two slots. *left can be the same as *right
 */
ETRANSFER swap_slots(entity_t *left, int pos_left, entity_t *right, int pos_right) {
	if (num_of_slots(left) < pos_left + 1 || pos_left < 0) {
		return OUT_OF_BOUNDS_LEFT;
	}
	if (num_of_slots(right) < pos_right + 1 || pos_right < 0) {
		return OUT_OF_BOUNDS_RIGHT;
	}

	slot_t *slot_left  = &(((basic_slot_t *) left->data )->slot[pos_left]);
	slot_t *slot_right = &(((basic_slot_t *) right->data)->slot[pos_right]);
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
	if (num_of_slots(left) < pos_left + 1 || pos_left < 0) {
		return OUT_OF_BOUNDS_LEFT;
	}
	if (num_of_slots(right) < pos_right + 1 || pos_right < 0) {
		return OUT_OF_BOUNDS_RIGHT;
	}

	slot_t *slot_left  = &(((basic_slot_t *) left->data )->slot[pos_left]);
	slot_t *slot_right = &(((basic_slot_t *) right->data)->slot[pos_right]);

	if (*slot_right != EMPTY) {
		return NO_SPACE;
	}

	*slot_right = *slot_left;
	*slot_left = EMPTY;

	return OK;
}
