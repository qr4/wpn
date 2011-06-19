#ifndef _JSON_OUTPUT_H
#define _JSON_OUTPUT_H

#include "entities.h"
#include "types.h"

/* Functions to create json-strings for ships, asteroids and bases.
 * Output from these functions already includes curlys around them. */
char* join(char** strings, char* sep);
char* bbox_to_json(void);
char* asteroid_to_json(entity_t* e);
char* base_to_json(entity_t* e);
char* planet_to_json(entity_t* e);
char* ship_to_json(entity_t* e);
char* asteroids_to_json();
char* bases_to_json();
char* planets_to_json();
char* ships_to_json();
char* explosion_to_json(entity_t* e);
void asteroids_to_network();
void bases_to_network();
void planets_to_network();
void ships_to_network();
void ship_updates_to_network(char** updated_ships, int updates);
void map_to_network();


#endif /* _JSON_OUTPUT_H */
