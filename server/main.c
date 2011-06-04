#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "entities.h"
#include "ship.h"
#include "luastate.h"
#include "map.h"
#include "config.h"
#include "json_output.h"
#include "entity_storage.h"
#include "storages.h"
#include "debug.h"
#include "../net/net.h"

#define print_sizeof(TYPE) \
	printf("sizeof(%-14s) = %4lu\n", #TYPE, sizeof(TYPE));

int main(int argc, char *argv[]) {
	ERROR("Error messages turned on!\n");
	DEBUG("Debug messages turned on!\n");

    /* init logging */
    log_open("test_net_main.log");
    log_msg("---------------- new start");

	entity_t *e;
	entity_t *closest;
	char* temp;

	/* Parse Config */
	config(argc, argv);

	/* Start networking code */
	net_init();

	/* Create entity storages */
	init_all_storages();

	init_map();
	/* Create a testship */
	vector_t v1 = vector(1000);
	entity_id_t ship1 = init_ship(ship_storage, v1, 6);

	/* Test json-output with the testship */
	temp = ship_to_json(get_entity_from_storage_by_id(ship_storage, ship1));
	DEBUG("In json, this is:\n%s\n", temp);
	free(temp);

	e = get_entity_by_id(ship1);

	/* Main simulation loop */
	for (uint64_t timestep=0;;timestep++) {

		/* Legacy Debugging Code, still left in */
		e->pos.v = (randv().v + vector(1).v) * vector(2000).v;
		closest = find_closest_by_position(e->pos, e->radius, 1000, PLANET);
		DEBUG("Checking (%f, %f)\n", e->pos.x, e->pos.y);
		if (closest != NULL) {
			DEBUG("Found %s, at position (%f, %f). Collision distance: %f\n",
					type_string(closest->type),
					closest->pos.x,
					closest->pos.y,
					collision_dist(&e, closest));
		} else {
			DEBUG("Nothing\n");
		}

		/* Iterate through all sentient entities (ships and bases) and determine if
		 * any timer-based callbacks should be triggered (like completed actions or
		 * expired explicit timers) */
		for(int i=0; i<ship_storage->first_free; i++) {
			if(ship_storage->entities[i].ship_data->timer_value == 0) {

				/* Set the timer to -1 (== disabled) */
				ship_storage->entities[i].ship_data->timer_value = -1;

				/* Call it's handler */
				DEBUG("Calling event handler!\n");
				call_entity_callback(&(ship_storage->entities[i]), ship_storage->entities[i].ship_data->timer_event);
			}
		}
		for(int i=0; i<base_storage->first_free; i++) {
			if(base_storage->entities[i].base_data->timer_value == 0) {

				/* Set the timer to -1 (== disabled) */
				base_storage->entities[i].base_data->timer_value = -1;

				/* Call it's handler */
				call_entity_callback(&(base_storage->entities[i]), base_storage->entities[i].base_data->timer_event);
			}
		}

		/* Decrement timers */
		for(int i=0; i<ship_storage->first_free; i++) {
			if(ship_storage->entities[i].ship_data->timer_value >= 0) {
				ship_storage->entities[i].ship_data->timer_value--;
			}
		}
		for(int i=0; i<base_storage->first_free; i++) {
			if(base_storage->entities[i].base_data->timer_value >= 0) {
				base_storage->entities[i].base_data->timer_value--;
			}
		}

		/* Likewise, notify all ships whose autopilots have arrived at their
		 * destinations */
		/* TODO: Implement autopilot arrival notification */

		/* Move ships by one physics-step */
		/* TODO: implement physics here. */

		/* Look for any collisions or proximity-triggers in spacecraft */
		/* TODO: Implement collisions */

		/* Push Map-Data out to clients */
		map_to_network();

		/* FIXME: This is just debug-Waiting. */
    sleep(1);
	}

	free_entity(ship_storage,ship1);

	free_all_storages();
	free_map();
	free_config();

	return EXIT_SUCCESS;
}
