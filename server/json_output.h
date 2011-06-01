#ifndef _JSON_OUTPUT_H
#define _JSON_OUTPUT_H

#include "entities.h"
#include "types.h"

/* Functions to create json-strings for ships, asteroids and bases.
 * Output from these functions already includes curlys around them. */
char* ship_to_json(entity_id_t id);
char* asteroid_to_json(entity_t* e);


#endif /* _JSON_OUTPUT_H */
