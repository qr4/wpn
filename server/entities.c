#include <stdio.h>
#include <string.h>

#include "types.h"
#include "entities.h"
#include "map.h"
#include "physics.h"
#include "luastate.h"
#include "debug.h"
#include "entity_storage.h"
#include "player.h"
#include "storages.h"
#include "json_output.h"
#include "../logging/logging.h"

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
	*slot_left = *slot_right;
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
			break;
		case ASTEROID :
			e->asteroid_data = (asteroid_data_t *) malloc (sizeof(asteroid_data_t));
			e->radius = asteroid_radius_to_slots_ratio * slots;
			e->asteroid_data->docked_to = INVALID_ID;
			break;
		case BASE :
			e->base_data     = (base_data_t *)     malloc (sizeof(base_data_t));
			e->base_data->timer_value = -1;
			e->base_data->timer_context = INVALID_ID;
			e->base_data->docked_to = INVALID_ID;
			break;
		case SHIP :
			e->ship_data     = (ship_data_t *)     malloc (sizeof(ship_data_t));
			e->ship_data->flightplan = NULL;
			e->ship_data->timer_value = -1;
			e->ship_data->timer_context = INVALID_ID;
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
			if(type != ASTEROID) {
				e->slot_data->slot[i] = EMPTY;
			} else {
				double rnd = 100 * drand48();
				rnd -= config_get_double("initial_asteroid_drive");
				if(rnd < 0) {
					e->slot_data->slot[i] = DRIVE;
					continue;
				}
				rnd -= config_get_double("initial_asteroid_weapon");
				if(rnd < 0) {
					e->slot_data->slot[i] = WEAPON;
					continue;
				}
				rnd -= config_get_double("initial_asteroid_ore");
				if(rnd < 0) {
					e->slot_data->slot[i] = ORE;
					continue;
				}
				e->slot_data->slot[i] = EMPTY;
			}
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
		e->lua = NULL;
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
	waypoint_t* next = ship->ship_data->flightplan->next;
	free_waypoint(ship->ship_data->flightplan);
	ship->ship_data->flightplan = next;

	if(next) {
		quad_index_t old_quad = get_quad_index_by_pos(ship->pos);
		if(next->point.x < map.left_bound || next->point.x > map.right_bound || next->point.y < map.upper_bound || next->point.y > map.lower_bound) {
			log_msg("%d is outside the map\n", ship->unique_id);
			explode_entity(ship);
			return;
		}
		ship->pos.v = next->point.v;
		quad_index_t new_quad = get_quad_index_by_pos(ship->pos);
		if((old_quad.quad_x != new_quad.quad_x) || (old_quad.quad_y != new_quad.quad_y)) {
			update_quad_object(ship);
		}

		ship->v.v = next->speed.v;
	} else {
		ship->v.v = (v2d) {0, 0};
	}
	current_ships = realloc(current_ships, sizeof(char*) * (n_current_ships+2));
	if(!current_ships) {
		ERROR("Out of memory for ship movement json-strings.");
		return;
	}
	current_ships[n_current_ships] = ship_to_json(ship);
	n_current_ships++;
}

/* And boom goes the entity! */
/* (Note: this removes the entity from it's storage object. No accesses via the
 * entity pointer should be performed after this function call) */
void explode_entity(entity_t* e) {

	player_data_t* p;

	if(!e) {
		ERROR("Attempting to explode a NULL entity!");
		return;
	}

	/* Only ships and bases should be able to explode */
	if(!(e->type & (BASE|SHIP))) {
		ERROR("Attempting to explode something not a ship or a base!");
		return;
	}

	/* Record the explosion-json-update for later */
	current_explosions = realloc(current_explosions, sizeof(char*) * (n_current_explosions+2));
	if(!current_explosions) {
		ERROR("Out of memory for explosion json-strings.");
		return;
	}
	current_explosions[n_current_explosions++] = explosion_to_json(e);
	current_explosions[n_current_explosions] = NULL;

	/* Care must be taken when destroying a player's homebase.
	 * He will get a new one right away. */
	p=find_player(e->player_id);
	if(p && p->homebase.id == e->unique_id.id) {
		p->homebase=INVALID_ID;
		create_homebase(p);

		/* Re-load the player base code into it */
		evaluate_player_base_code(p, p->homebase);

		/* Inform him! */
		call_entity_callback(get_entity_by_id(p->homebase), HOMEBASE_KILLED, p->homebase);
	}

	/* If we are a base, free our planet */
	if(e->type == BASE) {
		if(!e->base_data || !get_entity_by_id(e->base_data->my_planet)) {
			ERROR("Base lacking a planet!");
		}
		get_entity_by_id(e->base_data->my_planet)->player_id = 0;
	}

	/* Remove from docking partner */
	if(e->ship_data && (e->ship_data->docked_to.id != INVALID_ID.id)) {
		entity_t* other = get_entity_by_id(e->ship_data->docked_to);
		if(other) {
			other->ship_data->docked_to = INVALID_ID;
		}
	}

	/* Remove the exploding entity */
	switch(e->type) {
		case SHIP:
			free_entity(ship_storage, e->unique_id);
			return;
		case BASE:
			free_entity(base_storage, e->unique_id);
			return;
		default:
			ERROR("How the hell did we end up HERE? Your computer is broken.");
			return;
	}
}

/* Setup a timer for this entity, informing it when a certain action is complete */
void set_entity_timer(entity_t* e, int timesteps, event_t event, entity_id_t context) {

	if(!e) {
		ERROR("Attempt to set time on NULL\n");
		return;
	}

	/* Only sentient entities can have timers */
	switch(e->type) {
	case SHIP:
	case BASE:
		break;
	default:
		ERROR("Attempted to set a timer on a non-sentient entity!\n");
		return;
	}

	if(!e->ship_data) {
		ERROR("Ships really ought to have ship_data. Go and fix this urs\n");
		return;
	}

	if(e->ship_data->timer_value != -1) {
		ERROR("Another timer was already ticking for entity %lu\n",  e->unique_id.id);
		return;
	}

	e->ship_data->timer_value = timesteps;
	e->ship_data->timer_event = event;
	e->ship_data->timer_context = context;

}

/* Determine whether an entity is currently waiting for some action to finish, or just
 * idling around. */
int is_busy(entity_t* e) {

	switch(e->type) {
		case SHIP:
		case BASE:
			return e->ship_data->timer_value != -1;
		default:
			return 0;
	}
}
