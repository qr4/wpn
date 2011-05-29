#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "entities.h"
#include "json_output.h"

extern map_t map;

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

	for(i = 0; i < stringno; i++) {
		strcat(retval, strings[i]);
		if(i < stringno-1) {
			strcat(retval, sep);
		}
	}

	return retval;
}

/* Give the json representation of a ship (with the curly braces included) */
/* TODO: docked_to currently always contains null */
char* ship_to_json(entity_t* e) {

	char* contents;
	char* retval;

	if(!e) {
		fprintf(stderr, "Trying to json-ize a nullpointer in ship_to_json\n");
		return NULL;
	} else if(e->type != SHIP) {
		fprintf(stderr, "Trying to json-ize something that is not a ship in ship_to_json\n");
		return NULL;
	}

	contents = slots_to_string(e);

	asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": %i, \"size\": %i, \"contents\": \"%s\", \"docked_to\":null}",
			(uint64_t)e, e->pos.x, e->pos.y, e->player_id, e->slots, contents);

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
	} else if(e->type != ASTEROID) {
		fprintf(stderr, "Trying to json-ize something that is not an astroid in astroid_to_json\n");
		return NULL;
	}

	contents = slots_to_string(e);

	asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"contents\": \"%s\"}",
			(uint64_t)e, e->pos.x, e->pos.y, contents);

	free(contents);

	return retval;
}


/* Returns the json representation of the given base (with curvy braces included) */
/* TODO: docked_to currently always contains null */
char* base_to_json(entity_t* e) {

	char* contents;
	char* retval;

	if(!e) {
		fprintf(stderr, "Trying to json-ize a nullpointer in base_to_json\n");
		return NULL;
	} else if(e->type != BASE) {
		fprintf(stderr, "Trying to json-ize something that is not a base in base_to_json\n");
		return NULL;
	}

	contents = slots_to_string(e);

	asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": %i, \"size\": %i, \"contents\": \"%s\", \"docked_to\":null}",
			(uint64_t)e, e->pos.x, e->pos.y, e->player_id, e->slots, contents);

	free(contents);

	return retval;
}


/* Returns the json representation of the bbox (with curvy braces included) */
char* bbox_to_json(void) {

	char* retval;

	asprintf(&retval, "{\"xmin\": %f, \"xmax\": %f, \"ymin\": %f, \"ymax\": %f }", map.left_bound, map.right_bound, map.upper_bound, map.lower_bound);

	return retval;
}


/* Returns the json representation of the given planet (with curvy braces included) */
char* planet_to_json(entity_t* e) {

	char* retval;

	if(!e) {
		fprintf(stderr, "Trying to json-ize a nullpointer in planet_to_json\n");
		return NULL;
	} else if(e->type != PLANET) {
		fprintf(stderr, "Trying to json-ize something that is not an planet in planet_to_json\n");
		return NULL;
	}

	asprintf(&retval, "{\"id\": %li, \"x\": %f, \"y\": %f, \"owner\": null}",
			(uint64_t)e, e->pos.x, e->pos.y);

	return retval;
}

char* planets_to_json() {
	int no_planets = 0;
	char* retval;
	char** planet_strings = malloc(map.clusters_x * map.clusters_y*sizeof(char*));

	//asprintf(&retval, "{\"planets\" ");

	int i;
	for(i = 0; i < map.clusters_x * map.clusters_y; i++) {
		if(map.cluster->cluster_data->asteroids == 0) { // zero asteroids so it must contain a planet
			planet_strings[no_planets] = planet_to_json(map.cluster->cluster_data->planet);
			no_planets++;
		}
	}

	if(no_planets <= 0) {
		asprintf(&retval, "planets: []\n");
	} else {
		asprintf(&retval, "planets: [\n%s\n]\n", join(planet_strings, ", \n"));
	}

	for(i = 0; i < no_planets; i++) {
		free(planet_strings[i]);
	}

	return retval;
}
