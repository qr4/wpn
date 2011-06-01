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

#define print_sizeof(TYPE) \
	printf("sizeof(%-14s) = %4lu\n", #TYPE, sizeof(TYPE));

int main(int argc, char *argv[]) {
	ERROR("Error messages turned on!\n");
	DEBUG("Debug messages turned on!\n");

	int i;
	entity_t e;
	entity_t *closest;
	char* temp;

	/* Parse Config */
	if(!init_config_from_file("config.lua")) {
		exit(1);
	}

	/* Create entity storages */
	init_all_storages();

	init_map();
	/* Create a testship */
	vector_t v1 = vector(1000);
	entity_id_t ship1 = init_ship(ship_storage, v1, 6);

	/* Test json-output with the testship */
	temp = ship_to_json(ship1);
	DEBUG("In json, this is:\n%s\n", temp);
	free(temp);

	/* Test freeing of this ship */
	//destroy_entity(ship1);
	free_entity(ship_storage,ship1);

	e.radius = 1;

	for (i = 0; i < 5; i++) {
		e.pos.v = (randv().v + vector(1).v) * vector(2000).v;
		closest = find_closest_by_position(e.pos, e.radius, 1000, CLUSTER);
		DEBUG("Checking (%f, %f)\n", e.pos.x, e.pos.y);
		if (closest != NULL) {
			DEBUG("Found %s, at position (%f, %f). Collision distance: %f\n",
					type_string(closest->type),
					closest->pos.x,
					closest->pos.y,
					collision_dist(&e, closest));
		} else {
			DEBUG("Nothing\n");
		}
	}

	free_config();
	free_map();

	return EXIT_SUCCESS;
}
