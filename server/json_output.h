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
char* player_to_json(player_data_t* p);
char* players_to_json();
char* shot_to_json(entity_t* source, entity_t* target);
void send_base_update(entity_t* e);
void send_planet_update(entity_t* e);
void asteroids_to_network();
void bases_to_network();
void planets_to_network();
void ships_to_network();
void ship_updates_to_network();
void explosions_to_network();
void shots_to_network();
void players_to_network();
void player_updates_to_network();
void map_to_network();

/* Json strings of explosions in this timestep */
extern char** current_explosions;
extern size_t n_current_explosions;

/* Same for laser shots */
extern char** current_shots;
extern size_t n_current_shots;

/* Same for ship updates */
extern char** current_ships;
extern size_t n_current_ships;

#endif /* _JSON_OUTPUT_H */
