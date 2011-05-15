#ifndef MAP_H
#define MAP_H

#include "entities.h"
#include "vector.h"
#include "types.h"

extern map_t map;

void init_map();
void free_map();
void unregister_object(entity_t *e);
#endif  /*MAP_H*/
