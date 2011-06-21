#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/time.h>
#include <unistd.h>

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
#include "../net/talk.h"
#include "../logging/logging.h"
#include "physics.h"
#include "player.h"
#include <fenv.h>
#include <sys/types.h>
#include <signal.h>


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

extern int netpid;
extern int talkpid;

void kill_network(void) {
	if(netpid != -1) {
		fprintf(stderr, "Talk thread has pid %d\n", netpid);
		kill(netpid, SIGTERM);
	}
	if(talkpid != -1) {
		fprintf(stderr, "Talk thread has pid %d\n", talkpid);
		kill(talkpid, SIGTERM);
	}
	wait(NULL);
	wait(NULL);
}

int main(int argc, char *argv[]) {
	ERROR("Error messages turned on!\n");
	DEBUG("Debug messages turned on!\n");

	/* init logging */
	log_open("test_net_main.log");
	log_msg("---------------- new start");

	entity_t *e;
	entity_t *closest;
	char* temp;
	struct timeval t;
	struct timeval t_prev;
	struct timeval t_diff;

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

#ifdef ENABLE_DEBUG
	DEBUG("Listen to your mum kids. Never devide by zero. I'm watching you.\n");
	feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

	/* Start networking code */
	net_init();

	/* Create entity storages */
	init_all_storages();

	init_map();

	/* Allow users to connect */
	init_talk();

	/* set exit handler */
	atexit(kill_network);

	/* Create a test user and -ships */
	new_player(100);
	entity_id_t ship1;
	for(int i=0; i<200; i++)  {
		vector_t v1 = vector(i);
		ship1 = init_ship(ship_storage, v1, 6);
		e = get_entity_by_id(ship1);
		if(e) {
			e->player_id = 100;
			e->ship_data->slot[0] = DRIVE;
			register_object(e);
		}
	}

	map_to_network();

	/* Get initial time */
	gettimeofday(&t_prev, NULL);

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

				/* Call it's handler (not passing any entity) */
				call_entity_callback(&(ship_storage->entities[i]), ship_storage->entities[i].ship_data->timer_event, ship_storage->entities[i].ship_data->timer_context);
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

				/* Call it's handler (not passing any entity) */
				call_entity_callback(&(base_storage->entities[i]), base_storage->entities[i].base_data->timer_event, base_storage->entities[i].base_data->timer_context);
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
					call_entity_callback(&(ship_storage->entities[i]), AUTOPILOT_ARRIVED, INVALID_ID);
				}
			}
		}

		/* Look for any proximity-triggers in spacecraft */
		/* TODO: Implement proximity warnings */

		/* Move ships by one physics-step */
		char** updated_ships = malloc((ship_storage->first_free + 1)*sizeof(char*));
		if(!updated_ships) {
			fprintf(stderr, "Not enough ram for ship updates\n");
			exit(1);
		}
		int updates = 0;
		for(int i=0; i<ship_storage->first_free; i++) {
			if(ship_storage->entities[i].ship_data->flightplan != NULL) {
				waypoint_t* next = ship_storage->entities[i].ship_data->flightplan->next;
				free_waypoint(ship_storage->entities[i].ship_data->flightplan);
				ship_storage->entities[i].ship_data->flightplan = next;

				if(next) {
					quad_index_t old_quad = get_quad_index_by_pos(ship_storage->entities[i].pos);
					if(next->point.x < map.left_bound || next->point.x > map.right_bound || next->point.y < map.upper_bound || next->point.y > map.lower_bound) {
						log_msg("%d is outside the map\n", ship_storage->entities[i].unique_id);
						//explode(ship_storage->entities[i]);
					}
					ship_storage->entities[i].pos.v = next->point.v;
					quad_index_t new_quad = get_quad_index_by_pos(ship_storage->entities[i].pos);
					if((old_quad.quad_x != new_quad.quad_x) || (old_quad.quad_y != new_quad.quad_y)) {
						update_quad_object(&(ship_storage->entities[i]));
					}

					ship_storage->entities[i].v.v = next->speed.v;
				} else {
					ship_storage->entities[i].v.v = (v2d) {0, 0};
				}
				updated_ships[updates] = ship_to_json(&(ship_storage->entities[i]));
				updates++;
			}

			if(rand() < RAND_MAX/1000) {
				explode_entity(&(ship_storage->entities[i]));
				i--;
			}
		}
		// Send JSON to network and free strings
		ship_updates_to_network(updated_ships, updates);
		explosions_to_network();

		/* Look for any collisions in spacecraft */
		/* TODO: Implement collisions */

		/* Push Map-Data out to clients */
		//map_to_network();

		/* Check whether any new player-provided lua code has arrived and evaluate it */
		player_check_code_updates();

		/* Wait until we reach our frametime-limit */
		gettimeofday(&t, NULL);
		timersub(&t, &t_prev, &t_diff);
		while(t_diff.tv_sec < 1 && t_diff.tv_usec < config_get_int("frametime")) {
			usleep(1000);
			gettimeofday(&t, NULL);
			timersub(&t, &t_prev, &t_diff);
		}
		t_prev = t;
	}

	free_entity(ship_storage,ship1);

	free_all_storages();
	free_map();
	free_config();

	return EXIT_SUCCESS;
}
