#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
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
#include <sys/wait.h>
#include <unistd.h>


extern double MAXIMUM_PLANET_SIZE;

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

uint64_t timestep=0;
uint64_t time_of_last_map=0;

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
	//feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
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

	/* Load all known players into the simulation,
	 * and initialize their station code. */
	add_all_known_players();
	evaluate_all_player_code();

	map_to_network();

	/* Get initial time */
	gettimeofday(&t_prev, NULL);

	/* Main simulation loop */
	for (timestep=0;;timestep++) {
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
		for(int i=0; i<ship_storage->first_free; i++) {
			double scanner_range = config_get_double("scanner_range");
			double weapon_range = config_get_double("weapon_range");
			double range = scanner_range > weapon_range ? scanner_range : weapon_range;
			entity_t* self = &(ship_storage->entities[i]);
			size_t n;
			entity_id_t* neigh = get_entities(self->pos, range, SHIP|BASE, &n);
			for(int j=0; j<n; j++) {
				entity_t* other = get_entity_by_id(neigh[j]);
				vector_t oldpos_self;
				oldpos_self.v = self->pos.v - self->v.v * (v2d){dt, dt};
				vector_t oldpos_other;
				oldpos_other.v = other->pos.v - other->v.v * (v2d){dt, dt};
				if((vector_quaddist(&(self->pos), &(other->pos)) < scanner_range*scanner_range) &&
				   (vector_quaddist(&oldpos_self, &oldpos_other) > scanner_range*scanner_range)) {
					call_entity_callback(self, ENTITY_APPROACHING, other->unique_id);
				}
				if((vector_quaddist(&(self->pos), &(other->pos)) < weapon_range*weapon_range) &&
				   (vector_quaddist(&oldpos_self, &oldpos_other) > weapon_range*weapon_range)) {
					call_entity_callback(self, ENTITY_IN_RANGE, other->unique_id);
				}
			}
			free(neigh);
		}

		/* Move ships by one physics-step */
		char** updated_ships = malloc((ship_storage->first_free + 1)*sizeof(char*));
		if(!updated_ships) {
			fprintf(stderr, "Not enough ram for ship updates\n");
			exit(1);
		}

		for(int i=0; i<ship_storage->first_free; i++) {
			if(ship_storage->entities[i].ship_data->flightplan != NULL) {
				move_ship(ship_storage->entities + i);
			}
		}
		// Send JSON to network and free strings
		ship_updates_to_network();
		shots_to_network();
		explosions_to_network();

		/* Look for any collisions in spacecraft */
		for(int i=0; i<ship_storage->first_free; i++) {
			entity_t* e = &(ship_storage->entities[i]);
			entity_t* c = find_closest(e, 2*MAXIMUM_PLANET_SIZE, SHIP|ASTEROID|PLANET);
			if(!c) {
				continue;
			}
			double dist = collision_dist(e, c)+1;
			if(dist < 0) {
#ifdef ENABLE_DEBUG
				char* temp;
				temp = slots_to_string(e);
				DEBUG("{\n\tEntity %lu:\n\tpos: %f,  %f\n\tv: %f, %f\n\ttype: %s\n\tslots: %i\n\tplayer: %i\n\tcontents: [%s]\n\tradius: %f\n} might be colliding with \n", (size_t)e->unique_id.id, e->pos.x, e->pos.y, e->v.x, e->v.y, type_string(e->type), e->slots, e->player_id, temp, e->radius);
				free(temp);
				if(c->type == PLANET) {
					DEBUG("Planet belongs to cluster at (%f, %f), radius %f\n", c->planet_data->cluster->pos.x, c->planet_data->cluster->pos.y, c->planet_data->cluster->radius);
				} else if(c->type == ASTEROID) {
					DEBUG("Asteroid belongs to cluster at (%f, %f), radius %f\n", c->asteroid_data->cluster->pos.x, c->asteroid_data->cluster->pos.y, c->asteroid_data->cluster->radius);
				} else if(c->type == SHIP) {
					DEBUG("Another ship at (%f, %f), radius %f\n", c->pos.x, c->pos.y, c->radius);
				} else {
					DEBUG("Some other entity of type %d\n", c->type);
				}
				DEBUG("dist = %f\n\n\n\n", dist);
#endif
				explode_entity(e);
				if(c->type == SHIP) {
					explode_entity(c);
				}
			}
		}

		/* Push Map-Data out to clients */
		if(timestep - time_of_last_map > config_get_int("map_interval")) {
			map_to_network();
		}

		/* Check whether any new player-provided lua code has arrived and evaluate it */
		player_check_code_updates(0);

		/* Wait until we reach our frametime-limit */
		gettimeofday(&t, NULL);
		timersub(&t, &t_prev, &t_diff);
		while(t_diff.tv_sec < 1 && t_diff.tv_usec < config_get_int("frametime")) {
			player_check_code_updates(t_diff.tv_usec);
			gettimeofday(&t, NULL);
			timersub(&t, &t_prev, &t_diff);
		}
		t_prev = t;

		fprintf(stderr, ".");
	}

	//free_entity(ship_storage,ship1);

	free_all_storages();
	free_map();
	free_config();

	return EXIT_SUCCESS;
}
