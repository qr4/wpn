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
#include "physics.h"

#define print_sizeof(TYPE) \
	printf("sizeof(%-14s) = %4lu\n", #TYPE, sizeof(TYPE));

double dt = 0.5;
double vmax = 25;
double m0_small = 1;
double m0_medium = 2;
double m0_large = 4;
double m0_huge = 8;
double m0_klotz = 1;
double F_thruster = 20;
double epsilon = 1e-10;
double asteroid_radius_to_slots_ratio = 1;
double planet_size = 50;

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
	dt = config_get_double("dt");
	vmax = config_get_double("vmax");
	m0_small = config_get_double("m0_small");
	m0_medium = config_get_double("m0_medium");
	m0_large = config_get_double("m0_large");
	m0_huge = config_get_double("m0_huge");
	m0_klotz = config_get_double("m0_klotz");
	F_thruster = config_get_double("F_thruster");
	epsilon = config_get_double("epsilon");
	asteroid_radius_to_slots_ratio = config_get_double("asteroid_radius_to_slots_ratio");
	planet_size = config_get_double("planet_size");

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
	e->ship_data->slot[0] = DRIVE;

	/* Main simulation loop */
	for (uint64_t timestep=0;;timestep++) {

		/* Legacy Debugging Code, still left in */
		/*
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
		*/

		/* Iterate through all sentient entities (ships and bases) and determine if
		 * any timer-based callbacks should be triggered (like completed actions or
		 * expired explicit timers) */
		for(int i=0; i<ship_storage->first_free; i++) {
			if(ship_storage->entities[i].ship_data->timer_value == 0) {

				/* Set the timer to -1 (== disabled) */
				ship_storage->entities[i].ship_data->timer_value = -1;

				/* Call it's handler */
				call_entity_callback(&(ship_storage->entities[i]), ship_storage->entities[i].ship_data->timer_event);
			}

			/* Decrement timers */
			if(ship_storage->entities[i].ship_data->timer_value >= 0) {
				ship_storage->entities[i].ship_data->timer_value--;
			}
		}

		for(int i=0; i<base_storage->first_free; i++) {
			if(base_storage->entities[i].base_data->timer_value == 0) {

				/* Set the timer to -1 (== disabled) */
				base_storage->entities[i].base_data->timer_value = -1;

				/* Call it's handler */
				call_entity_callback(&(base_storage->entities[i]), base_storage->entities[i].base_data->timer_event);
			}

			/* Decrement timers */
			if(base_storage->entities[i].base_data->timer_value >= 0) {
				base_storage->entities[i].base_data->timer_value--;
			}
		}

		/* Likewise, notify all ships whose autopilots have arrived at their
		 * destinations */
		for(int i=0; i<ship_storage->first_free; i++) {
			if(ship_storage->entities[i].ship_data->flightplan != NULL) {
				if(ship_storage->entities[i].ship_data->flightplan->type == WP_STOP) {
					free_waypoint(ship_storage->entities[i].ship_data->flightplan);
					ship_storage->entities[i].ship_data->flightplan = NULL;
				}
			}
		}

		/* Look for any proximity-triggers in spacecraft */
		/* TODO: Implement proximity warnings */

		/* Move ships by one physics-step */
		char** updated_ships = malloc((ship_storage->first_free + 1)*sizeof(char*));
		int updates = 0;
		for(int i=0; i<ship_storage->first_free; i++) {
			if(ship_storage->entities[i].ship_data->flightplan != NULL) {
				waypoint_t* next = ship_storage->entities[i].ship_data->flightplan->next;
				free_waypoint(ship_storage->entities[i].ship_data->flightplan);
				ship_storage->entities[i].ship_data->flightplan = next;

				quad_index_t old_quad = get_quad_index_by_pos(ship_storage->entities[i].pos);
				ship_storage->entities[i].pos.v = next->point.v;
				quad_index_t new_quad = get_quad_index_by_pos(ship_storage->entities[i].pos);
				if((old_quad.quad_x != new_quad.quad_x) || (old_quad.quad_y != new_quad.quad_y)) {
					update_quad_object(&(ship_storage->entities[i]));
				}

				ship_storage->entities[i].v.v = next->speed.v;
				updated_ships[updates] = ship_to_json(&(ship_storage->entities[i]));
				updates++;
			}
		}
		// Send JSON to network and free strings
		ship_updates_to_network(updated_ships, updates);

		/* Look for any collisions in spacecraft */
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
