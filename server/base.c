#include <lua.h>
#include "luastate.h"
#include "base.h"
#include "vector.h"
#include "entity_storage.h"
#include "storages.h"

/* Create a new, empty base on some planet */
entity_id_t init_base(entity_storage_t* s, entity_id_t planet, uint8_t size) {

	entity_id_t id;
	entity_t* e, *eplanet;
	vector_t pos;

	eplanet = get_entity_by_id(planet);
	if(!eplanet) {
		ERROR("Planet does not exist! Not creating base!");
		return INVALID_ID;
	}
	pos = eplanet->pos;

	id = alloc_entity(s);
	e = get_entity_by_id(id);

	init_entity(e, pos, BASE, size);

	/* TODO: Calculate radius from size */
	e->radius = eplanet->radius;

	/* TODO: Initialize mass 'n stuff */
	e->player_id = 0;

	/* No event to start with */
	e->base_data->timer_value=-1;

	e->base_data->my_planet = planet;

	register_object(e);

	init_ship_computer(e);

	return id;
}

