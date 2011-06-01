#ifndef _ENTITY_STORAGE_H
#define _ENTITY_STORAGE_H

#include "entities.h"

typedef struct entity_storage entity_storage_t;

struct entity_storage {
	int max;
	entity_t* entities;
	uint32_t* offsets;
	uint32_t first_free;
};

/* Create a new, empty entity storage */
entity_storage_t* init_entity_storage(int n_entries);

/* Allocate a new entity from the storage pool */
entity_id_t alloc_entity(entity_storage_t* s);

/* Get an entity from its unique id. If no such entity exists, returns NULL */
entity_t* get_entity_by_id(entity_storage_t* s, entity_id_t id);

/* Get the i-th global entity */
entity_t* get_entity_by_index(entity_storage_t* s, uint32_t i);

/* Delete an entity from the storage, freeing its slot */
void free_entity(entity_storage_t* s, entity_id_t id);

void cleanup_storage(entity_storage_t* s);


/* And now, since we're so cute c-coders, the global array of entities. :) */
extern entity_storage_t* entities;

#endif /* _ENTITY_STORAGE_H */
