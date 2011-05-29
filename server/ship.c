#include <lua.h>
#include "luastate.h"
#include "ship.h"
#include "vector.h"

/* Create a new, empty ship */
void init_ship(entity_t *e, vector_t pos, uint8_t size) {

	//int i;

	init_entity(e, pos, SHIP, size);

	/* TODO: init content */
	//for(i=0; i<size; i++) {
	//	this->content[i] = ' ';
	//}
	init_ship_computer(e);
}
