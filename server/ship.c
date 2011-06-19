#include <lua.h>
#include "luastate.h"
#include "ship.h"
#include "vector.h"
#include "entity_storage.h"
#include "storages.h"

/* Create a new, empty ship */
entity_id_t init_ship(entity_storage_t* s, vector_t pos, uint8_t size) {

	entity_id_t id;
	entity_t* e;

	id = alloc_entity(s);
	e = get_entity_by_id(id);

	init_entity(e, pos, SHIP, size);

	/* TODO: Calculate radius from size */
	e->radius = 1;

	/* TODO: Initialize stuff */
	e->player_id = 0;

	/* TODO: init content */
	//for(i=0; i<size; i++) {
	//	this->content[i] = ' ';
	//}

	/* No event to start with */
	e->ship_data->timer_value=-1;

	init_ship_computer(e);

	return id;
}
