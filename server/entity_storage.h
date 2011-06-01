#ifndef _ENTITY_STORAGE_H
#define _ENTITY_STORAGE_H

#include "entities.h"

typedef struct entity_storage entity_storage_t;

struct entity_storage {
	entity_t* entities;
	uint32_t* offsets;
	uint32_t max;
	uint32_t first_free;
	uint16_t type;
};

/* Create a new, empty entity storage */
entity_storage_t* init_entity_storage(const uint32_t n_entries, const uint16_t type);

/* Allocate a new entity from the storage pool */
entity_id_t alloc_entity(entity_storage_t* s);

/* Get an entity from its unique id. If no such entity exists, returns NULL */
entity_t* get_entity_from_storage_by_id(entity_storage_t* s, entity_id_t id);

/* Get the i-th global entity */
entity_t* get_entity_by_index(entity_storage_t* s, uint32_t i);

/* Delete an entity from the storage, freeing its slot */
void free_entity(entity_storage_t* s, entity_id_t id);

void cleanup_storage(entity_storage_t* s);

#endif /* _ENTITY_STORAGE_H */
