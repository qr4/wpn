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

	int i;
	entity_t *e;
	entity_t *closest;
	char* temp;

	/* Parse Config */
	if(!init_config_from_file("config.lua")) {
		exit(1);
	}

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

	/* Test freeing of this ship */
	//destroy_entity(ship1);
	e = get_entity_by_id(ship1);

	for (i = 0; i < 5; i++) {
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

		map_to_network();

    sleep(5);
	}

	free_entity(ship_storage,ship1);

	free_config();
	free_all_storages();
	free_map();

	return EXIT_SUCCESS;
}
