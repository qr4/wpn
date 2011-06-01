#include "storages.h"
#include "config.h"
#include "debug.h"

/* The actual storage objects */
entity_storage_t* ship_storage;
entity_storage_t* base_storage;
entity_storage_t* asteroid_storage;
entity_storage_t* planet_storage;

/* initialize them! */
void init_all_storages() {

	int num_planets = config_get_int("num_planets");
	int num_asteroids = config_get_int("num_asteroids");
	int max_ships = config_get_int("max_ship_estimation");
	int max_bases = config_get_int("max_base_estimation");

	planet_storage=init_entity_storage(num_planets, PLANET);
	if(!planet_storage) {
		ERROR("Error creating planet storage. The Universe is full, go play somewhere else.\n");
		exit(1);
	}
	asteroid_storage=init_entity_storage(num_asteroids, ASTEROID);
	if(!asteroid_storage) {
		ERROR("Error creating asteroid storage. Couldn't find enough stones.\n");
		exit(1);
	}

	ship_storage=init_entity_storage(max_ships, SHIP);
	if(!ship_storage) {
		ERROR("Error creating ship storage. All these planets and asteroids are far too bulky\n");
		exit(1);
	}

	base_storage=init_entity_storage(max_bases, BASE);
	if(!base_storage) {
		ERROR("Error creating base storage. Too much crap floating around already.\n");
		exit(1);
	}
}

/* Get an entity of any type */
entity_t* get_entity_by_id(entity_id_t id) {

	switch(id.type) {
		case PLANET:
			return get_entity_from_storage_by_id(planet_storage, id);
		case ASTEROID:
			return get_entity_from_storage_by_id(asteroid_storage, id);
		case BASE:
			return get_entity_from_storage_by_id(base_storage, id);
		case SHIP:
			return get_entity_from_storage_by_id(ship_storage, id);
		default :
			DEBUG("Unknown entity type %i!\n", id.type);
			return NULL;
	}
}

