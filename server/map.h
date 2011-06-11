#ifndef MAP_H
#define MAP_H

#include "entities.h"
#include "vector.h"
#include "types.h"

extern map_t map;

void init_map();
void free_map();

static inline quad_index_t get_quad_index_by_pos(const vector_t pos) {
	return (quad_index_t) {(size_t) (pos.x / map.quad_size), (size_t) (pos.y / map.quad_size)};
}

static inline map_quad_t *get_quad_by_index(const quad_index_t quad_index) {
	return &(map.quad[map.quads_x * quad_index.quad_y + quad_index.quad_x]);
}

static inline map_quad_t *get_quad_by_pos(const vector_t pos) {
	return get_quad_by_index(get_quad_index_by_pos(pos));
}

map_quad_t *register_object(entity_t *e);
void unregister_object(entity_t *e);

entity_t *find_closest(entity_t *e, const double radius, const unsigned int filter);

static inline entity_t *find_closest_by_position(const vector_t pos, const double entity_radius, const double radius, const unsigned int filter) {
	entity_t tmp;
	tmp.pos = pos;
	tmp.radius = entity_radius;
	tmp.unique_id = INVALID_ID;
	return find_closest(&tmp, radius, filter);
}

#endif  /*MAP_H*/