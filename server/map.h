#ifndef MAP_H
#define MAP_H

#include "entities.h"
#include "vector.h"
#include "types.h"

extern map_t map;

void init_map();
void free_map();

map_quad_t *get_quad(entity_t *e);
map_quad_t *register_object(entity_t *e);
void unregister_object(entity_t *e);
#endif  /*MAP_H*/
