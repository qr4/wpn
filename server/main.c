#include <stdio.h>

#include "vector.h"
#include "entities.h"

#define print_sizeof(TYPE) \
	printf("sizeof(%-14s) = %4lu\n", #TYPE, sizeof(TYPE));

int main(int argc, char *argv[]) {
	print_sizeof(type_t);
	print_sizeof(entity_t);

	printf("\nShips:\n");

	print_sizeof(SHIP_S_t);
	print_sizeof(SHIP_M_t);
	print_sizeof(SHIP_L_t);
	print_sizeof(SHIP_XL_t);
	print_sizeof(SHIP_XXL_t);

	printf("\nPlanets:\n");

	print_sizeof(PLANET_S_t);
	print_sizeof(PLANET_M_t);
	print_sizeof(PLANET_L_t);
	print_sizeof(PLANET_XL_t);
	print_sizeof(PLANET_XXL_t);

	printf("\nAsteroids:\n");

	print_sizeof(ASTEROID_S_t);
	print_sizeof(ASTEROID_M_t);
	print_sizeof(ASTEROID_L_t);
	print_sizeof(ASTEROID_XL_t);
	print_sizeof(ASTEROID_XXL_t);
	return 0;
	return 0;
}
