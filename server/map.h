#ifndef MAP_H
#define MAP_H

#include "entities.h"
#include "vector.h"
#include "types.h"

map_t *generate_map();
void free_map(map_t *map);

void unregister_object(map_t *map, entity_t *e);
#endif  /*MAP_H*/
