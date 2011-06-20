#include <lua.h>
#include "luastate.h"
#include "base.h"
#include "vector.h"
#include "entity_storage.h"
#include "storages.h"

/* Create a new, empty base on some planet */
entity_id_t init_base(entity_storage_t* s, entity_id_t planet, uint8_t size) {

	entity_id_t id;
	entity_t* e;
	vector_t pos;

	pos = get_entity_by_id(planet)->pos;

	id = alloc_entity(s);
	e = get_entity_by_id(id);

	init_entity(e, pos, BASE, size);

	/* TODO: Calculate radius from size */
	e->radius = 1;

	/* TODO: Initialize mass 'n stuff */
	e->player_id = 0;

	/* No event to start with */
	e->base_data->timer_value=-1;

	init_ship_computer(e);

	return id;
}
