#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "entities.h"
#include "ship.h"
#include "luastate.h"
#include "map.h"

#define print_sizeof(TYPE) \
	printf("sizeof(%-14s) = %4lu\n", #TYPE, sizeof(TYPE));

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

int main(int argc, char *argv[]) {
	//vector_t v1 = vector(5);
	//vector_t v2 = vector(0);
	//v2.x = 5;
	//v1.v += v2.v;

	//printf("v1 x: %f, y: %f\n", v1.x, v1.y);
	//printf("v2 x: %f, y: %f\n", v2.x, v2.y);
	//printf("dist(v1, v2): %f\n", vector_dist(&v1, &v2));
	//print_sizeof(vector_t);
	//print_sizeof(entity_t);

	//printf("\n\n");

	//entity_t* ship1, *asteroid1;
	//ship1 = create_ship(v1, 6);

	//asteroid1 = malloc(sizeof(entity_t));
	//asteroid1->slots = 4;
	//asteroid1->type = ASTEROID;
	//asteroid1->radius = 0.5;
	//asteroid1->pos.x = 5;
	//asteroid1->pos.y = 2;

	///* Main loop */
	//int i;
	//for (i = 0; i < 20; i++) {

	//	/* TODO: Evaluate ship's flight plan */

	//	/* If the ship is resting, tell it that it's arrived somewhere */
	//	if(ship1->v.x == 0 && ship1->v.y == 0) {
	//		call_entity_callback(ship1, AUTOPILOT_ARRIVED);
	//	}

	//	move_ship(ship1);            // move the ship one physicstep further

	//	/* Check distances, call callbacks if required */
	//	printf("ship x: %.2f, y: %.2f; collision_distance(ship, asteroid): %.2f\n", ship1->pos.x, ship1->pos.y, collision_dist(ship1, asteroid1));
	//	if(collision_dist(ship1, asteroid1) < 3.) {
	//		call_entity_callback(ship1, ENTITY_APPROACHING);
	//	}

	//	/* Check timers, call callbacks if required */
	//}

	//printf("ship x: %.2f, y: %.2f\n", ship1->pos.x, ship1->pos.y);

	int i;
	entity_t e;
	entity_t *closest;

	e.radius = 1;

	init_map();

	for (i = 0; i < 100; i++) {
		e.pos.v = (randv().v + vector(1).v) * vector(2000).v;
		closest = find_closest(&e, 1000, CLUSTER);
		printf("Checking (%f, %f)\n", e.pos.x, e.pos.y);
		if (closest != NULL) {
			printf("Found %s, at position (%f, %f). Collision distance: %f\n",
					type_string(closest->type),
					closest->pos.x,
					closest->pos.y,
					collision_dist(&e, closest));
		} else {
			printf("Nothing\n");
		}
	}

	free_map();

	return EXIT_SUCCESS;
}
