#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include "entities.h"
#include "json_output.h"

/* Give the json representation of a ship (with the curly braces included) */
/* TODO: docked_to currently always contains null */
char* ship_to_json(entity_t* e) {
	
	char* contents;
	char* retval;

	if(!e) {
		fprintf(stderr, "Trying to json-ize a nullpointer in ship_to_json\n");
		return NULL;
	}

	contents = slots_to_string(e);

	if(asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": %i, \"size\": %i, \"contents\": \"%s\", \"docked_to\":null}",
			(uint64_t)e, e->pos.x, e->pos.y, e->player_id, e->slots, contents));

	free(contents);

	return retval;
}


/* Returns the json representation of the given asteroid (with curvy braces included) */
char* asteroid_to_json(entity_t* e) {
	
	char* contents;
	char* retval;

	if(!e) {
		fprintf(stderr, "Trying to json-ize a nullpointer in asteroid_to_json\n");
		return NULL;
	}

	contents = slots_to_string(e);

	if(asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"contents\": \"%s\"}",
			(uint64_t)e, e->pos.x, e->pos.y, contents));

	free(contents);

	return retval;
}
