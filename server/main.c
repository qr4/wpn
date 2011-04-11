#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "entities.h"
#include "luastate.h"

#define print_sizeof(TYPE) \
	printf("sizeof(%-14s) = %4lu\n", #TYPE, sizeof(TYPE));

int main(int argc, char *argv[]) {
	vector_t v1 = vector(5);
	vector_t v2 = vector(0);
	v2.x = 5;
	v1.v += v2.v;

	printf("v1 x: %f, y: %f\n", v1.x, v1.y);
	printf("v2 x: %f, y: %f\n", v2.x, v2.y);
	printf("dist(v1, v2): %f\n", vector_dist(&v1, &v2));
	print_sizeof(vector_t);
	print_sizeof(entity_t);

	printf("\n\n");

	entity_t* ship1, *asteroid1;
	ship1 = create_ship(v1, 6);
	//ship1.type.slots = 3;
	//ship1.type.ship = 1;
	//ship1.radius = 1;
	//ship1.pos = vector(0);            // set both x and y position to 0
	//ship1.v.v = (v2d)       {10, 0};  // set x speed to 10, y speed to 0
	//ship1.v   = (vector_t) {{10, 0}}; // this is equivalent

	asteroid1 = malloc(sizeof(entity_t));
	asteroid1->type.slots = 4;
	asteroid1->type.asteroid = 1;
	asteroid1->radius = 0.5;
	asteroid1->pos.x = 5;
	asteroid1->pos.y = 2;

	/* Main loop */
	int i;
	for (i = 0; i < 20; i++) {

		/* TODO: Evaluate ship's flight plan */

		/* If the ship is resting, tell it that it's arrived somewhere */
		if(ship1->v.x == 0 && ship1->v.y == 0) {
			call_entity_callback(ship1, AUTOPILOT_ARRIVED);
		}

		move_ship(ship1);            // move the ship one physicstep further

		/* Check distances, call callbacks if required */
		printf("ship x: %.2f, y: %.2f; collision_distance(ship, asteroid): %.2f\n", ship1->pos.x, ship1->pos.y, collision_dist(ship1, asteroid1));
		if(collision_dist(ship1, asteroid1) < 3.) {
			call_entity_callback(ship1, ENTITY_APPROACHING);
		}

		/* Check timers, call callbacks if required */
	}

	printf("ship x: %.2f, y: %.2f\n", ship1->pos.x, ship1->pos.y);

	return EXIT_SUCCESS;
}
