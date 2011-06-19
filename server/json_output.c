#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "debug.h"
#include "entities.h"
#include "entity_storage.h"
#include "json_output.h"
#include "storages.h"
#include "../net/net.h"

extern entity_storage_t* asteroid_storage;
extern entity_storage_t* base_storage;
extern entity_storage_t* planet_storage;
extern entity_storage_t* ship_storage;

// Joins strings separated by sep until strings contain a NULL element
// Arguments and return string need to be freed by the caller
char* join(char** strings, char* sep) {
	int buflen = 0;
	int stringno = 0;
	char* retval;
	int i;

	while(strings[stringno] != NULL) {
		buflen += strlen(strings[stringno]);
		stringno++;
	}


	buflen += (stringno - 1) * strlen(sep);
	buflen += 1; //NULL Byte

	if(!(retval = malloc(buflen))) {
		fprintf(stderr, "Allocating composite string failed\n");
		exit(1);
	}

	memset(retval, 0, buflen);

	for(i = 0; i < stringno; i++) {
		strcat(retval, strings[i]);
		if(i < stringno-1) {
			strcat(retval, sep);
		}
	}

	return retval;
}

/* Returns the json representation of the bbox (with curvy braces included) */
char* bbox_to_json(void) {
	char* retval;

	asprintf(&retval, "\"bounding-box\": {\"xmin\": %f, \"xmax\": %f, \"ymin\": %f, \"ymax\": %f }", map.left_bound, map.right_bound, map.upper_bound, map.lower_bound);

	return retval;
}

/* Returns the json representation of the given asteroid (with curvy braces included) */
char* asteroid_to_json(entity_t* e) {
	char* contents;
	char* retval;

	if(!e) {
		DEBUG("Trying to json-ize a NULL pointer in asteroid_to_json\n");
		return NULL;
	} else if(e->type != ASTEROID) {
		DEBUG("not an astroid\n");
		return NULL;
	}

	contents = slots_to_string(e);

	asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"contents\": \"%s\"}",
			(uint64_t)e, e->pos.x, e->pos.y, contents);

	free(contents);

	return retval;
}

/* Returns the json representation of the given base (with curvy braces included) */
char* base_to_json(entity_t* e) {
	char* contents;
	char* retval;

	if(!e) {
		DEBUG("Trying to json-ize a NULL Pointer in base_to_json\n");
		return NULL;
	} else if(e->type != BASE) {
		DEBUG("not a base\n");
		return NULL;
	}

	contents = slots_to_string(e);

	if(e->base_data->docked_to.id == INVALID_ID.id) {
		asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": %i, \"size\": %i, \"contents\": \"%s\", \"docked_to\":null}",
			e->unique_id.id, e->pos.x, e->pos.y, e->player_id, e->slots, contents);
	} else {
		asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": %i, \"size\": %i, \"contents\": \"%s\", \"docked_to\": %lu}",
			e->unique_id.id, e->pos.x, e->pos.y, e->player_id, e->slots, contents, e->base_data->docked_to.id);
	}

	free(contents);

	return retval;
}

/* Returns the json representation of the given planet (with curvy braces included) */
char* planet_to_json(entity_t* e) {
	char* retval;

	if(!e) {
		DEBUG("Trying to json-ize a NULL pointer in planet_to_json\n");
		return NULL;
	} else if(e->type != PLANET) {
		DEBUG("not a planet\n");
		return NULL;
	}

	asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": null}",
			e->unique_id.id, e->pos.x, e->pos.y);

	return retval;
}

/* Give the json representation of a ship (with the curly braces included) */
char* ship_to_json(entity_t* e) {
	char* contents;
	char* retval;

	if(!e) {
		DEBUG("Trying to json-ize a NULL pointer in ship_to_json\n");
		return NULL;
	} else if(e->type != SHIP) {
		DEBUG("not a ship\n");
		return NULL;
	}

	contents = slots_to_string(e);

	if(e->ship_data->docked_to.id == INVALID_ID.id) {
		asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": %i, \"size\": %i, \"contents\": \"%s\", \"docked_to\":null}",
			e->unique_id.id, e->pos.x, e->pos.y, e->player_id, e->slots, contents);
	} else {
		asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": %i, \"size\": %i, \"contents\": \"%s\", \"docked_to\": %lu}",
			e->unique_id.id, e->pos.x, e->pos.y, e->player_id, e->slots, contents, e->ship_data->docked_to.id);
	}

	free(contents);

	return retval;
}

/* Give the json representation of an explosion of the given entity (with curly braces) */
char* explosion_to_json(entity_t* e) {
	char* retval;

	if(!e) {
		DEBUG("Trying to explode a NULL entity in explosion_to_json\n");
		return NULL;
	} else if(!(e-> type & (BASE|SHIP))) {
		DEBUG("Trying to explode something that's neither a base nor a ship.\n");
		return NULL;
	}

	asprintf(&retval, "{\"exploding_entity\": %li, \"x\": %f, \"y\": %f}", e->unique_id.id, e->pos.x, e->pos.y);

	return retval;
}

char* asteroids_to_json() {
	int num_asteroids = asteroid_storage->first_free;
	char* retval;
	char** asteroid_strings = malloc((num_asteroids+1)*sizeof(char*));
	if(!asteroid_strings) {
		fprintf(stderr, "No space left for asteroids descripitons\n");
		exit(1);
	}

	int i;
	for(i = 0; i < num_asteroids; i++) {
		asteroid_strings[i] = asteroid_to_json(get_entity_by_index(asteroid_storage, i));
	}
	asteroid_strings[num_asteroids] = NULL;

	if(num_asteroids <= 0) {
		asprintf(&retval, "\"asteroids\": []\n");
	} else {
		char* p = join(asteroid_strings, ",\n");
		asprintf(&retval, "\"asteroids\": [\n%s\n]\n", p);
		free(p);
	}

	for(i = 0; i < num_asteroids; i++) {
		free(asteroid_strings[i]);
	}

	free(asteroid_strings);

	return retval;
}

char* bases_to_json() {
	int num_bases = base_storage->first_free;
	char* retval;
	char** base_strings = malloc((num_bases+1)*sizeof(char*));
	if(!base_strings) {
		fprintf(stderr, "No space left for base descripitons\n");
		exit(1);
	}

	int i;
	for(i = 0; i < num_bases; i++) {
		base_strings[i] = base_to_json(get_entity_by_index(base_storage, i));
	}
	base_strings[num_bases] = NULL;

	if(num_bases <= 0) {
		asprintf(&retval, "\"bases\": []\n");
	} else {
		char* p = join(base_strings, ",\n");
		asprintf(&retval, "\"bases\": [\n%s\n]\n", p);
		free(p);
	}

	for(i = 0; i < num_bases; i++) {
		free(base_strings[i]);
	}

	free(base_strings);

	return retval;
}

char* planets_to_json() {
	int num_planets = planet_storage->first_free;
	char* retval;
	char** planet_strings = malloc((num_planets+1)*sizeof(char*));
	if(!planet_strings) {
		fprintf(stderr, "No space left for planet descripitons\n");
		exit(1);
	}

	int i;
	for(i = 0; i < num_planets; i++) {
		planet_strings[i] = planet_to_json(get_entity_by_index(planet_storage, i));
	}
	planet_strings[num_planets] = NULL;

	if(num_planets <= 0) {
		asprintf(&retval, "\"planets\": []\n");
	} else {
		char* p = join(planet_strings, ",\n");
		asprintf(&retval, "\"planets\": [\n%s\n]\n", p);
		free(p);
	}

	for(i = 0; i < num_planets; i++) {
		free(planet_strings[i]);
	}

	free(planet_strings);

	return retval;
}

char* ships_to_json() {
	int num_ships = ship_storage->first_free;
	char* retval;
	char** ship_strings = malloc((num_ships+1)*sizeof(char*));
	if(!ship_strings) {
		fprintf(stderr, "No space left for ship descripitons\n");
		exit(1);
	}

	int i;
	for(i = 0; i < num_ships; i++) {
		ship_strings[i] = ship_to_json(get_entity_by_index(ship_storage, i));
	}
	ship_strings[num_ships] = NULL;

	if(num_ships <= 0) {
		asprintf(&retval, "\"ships\": []\n");
	} else {
		char* p = join(ship_strings, ",\n");
		asprintf(&retval, "\"ships\": [\n%s\n]\n", p);
		free(p);
	}

	for(i = 0; i < num_ships; i++) {
		free(ship_strings[i]);
	}

	free(ship_strings);

	return retval;
}

void asteroids_to_network() {
	char* p = asteroids_to_json();
	if(p) {
		map_printf("%s", p);
		free(p);
	} else {
		fprintf(stderr, "asteroids_to_network says: printing NULL is hard\n");
		exit(1);
	}
}

void bases_to_network() {
	char* p = bases_to_json();
	if(p) {
		map_printf("%s", p);
		free(p);
	} else {
		fprintf(stderr, "bases_to_network says: printing NULL is hard\n");
		exit(1);
	}
}

void bbox_to_network() {
	char* p = bbox_to_json();
	if(p) {
		map_printf("%s", p);
		free(p);
	} else {
		fprintf(stderr, "bbox_to_network says: printing NULL is hard\n");
		exit(1);
	}
}

void planets_to_network() {
	char* p = planets_to_json();
	if(p) {
		map_printf("%s", p);
		free(p);
	} else {
		fprintf(stderr, "planets_to_network says: printing NULL is hard\n");
		exit(1);
	}
}

void ships_to_network() {
	char* p = ships_to_json();
	if(p) {
		map_printf("%s", p);
		free(p);
	} else {
		fprintf(stderr, "ships_to_network says: printing NULL is hard\n");
		exit(1);
	}
}

void ship_updates_to_network(char** updated_ships, int updates) {
	char* joined_updates;
	updated_ships[updates] = NULL;

	if(updates <= 0) {
		asprintf(&joined_updates, "\"ships\": []\n");
	} else {
		char* p = join(updated_ships, ",\n");
		asprintf(&joined_updates, "\"ships\": [\n%s\n]\n", p);
		free(p);
	}

	for(int i = 0; i < updates; i++) {
		free(updated_ships[i]);
	}

	free(updated_ships);

	update_printf("{ \"update\":\n{ %s}\n}\n\n", joined_updates);
	update_flush();
	free(joined_updates);
}

void map_to_network() {
	map_printf("{ \"world\":\n{ ");
	bbox_to_network();
	map_printf(",\n");
	asteroids_to_network();
	map_printf(",\n");
	bases_to_network();
	map_printf(",\n");
	planets_to_network();
	map_printf(",\n");
	ships_to_network();
	map_printf("}\n}\n\n");
	map_flush();
}
