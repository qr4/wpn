#include "entity_storage.h"
#include "entities.h"
#include "debug.h"

/* Create a new, empty entity storage, with the specified starting size and
 * type bitmask added to the entities' ids */
entity_storage_t* init_entity_storage(const uint32_t n_entries, const uint16_t type) {
	int i;

	/* Allocate space for the storage itself */
	entity_storage_t* s = malloc(sizeof(entity_storage_t));
	
	/* If no default size has been specified, just assume... something */
	if (n_entries == 0) {
		s->max = 32;
	} else {
		s->max = n_entries;
	}

	/* Allocate space for the entries */
	s->entities=malloc(sizeof(entity_t) * s->max);
	s->offsets=malloc(sizeof(uint32_t) * s->max);
	s->type = type;

	/* Initialize offset indices in a consistent way */
	for (i = 0; i < s->max; i++) {
		s->offsets[i]=i;
		s->entities[i].unique_id = (entity_id_t){{.index=i, .reincarnation=0, .type = s->type}};
	}
	
	s->first_free = 0;

	return s;
}

/* Allocate a new entity from the storage pool */
entity_id_t alloc_entity(entity_storage_t* s) {

	entity_t* retval;
	int i;

	/* Sanity check: Full? */
	if (s->first_free >= s->max) {
		DEBUG("EXPANDING %s\n", type_string(s->type));
		s->max *= 2;
		s->offsets  = realloc(s->offsets,  sizeof(uint32_t) * s->max);
		s->entities = realloc(s->entities, sizeof(entity_t) * s->max);

		for (i = s->first_free; i < s->max; i++) {
			s->offsets[i]=i;
			s->entities[i].unique_id = (entity_id_t) {{.index = i, .reincarnation = 0, .type = s->type}};
		}
		

	}

	retval = &(s->entities[s->first_free]);

	s->first_free++;

	DEBUG("Added id: %lu {.index = %u, .reincarnation = %u, .type = %u}\n", 
			retval->unique_id.id,
			retval->unique_id.index,
			retval->unique_id.reincarnation,
			retval->unique_id.type);

	return retval->unique_id;
}

/* Get an entity from its unique id. If no such entity exists, returns NULL */
entity_t* get_entity_from_storage_by_id(entity_storage_t* s, entity_id_t id) {

	uint32_t index = id.index;

	if(index >= s->max) {
		/* Entity out of bounds for this storage. Well, what do we care. */
		DEBUG("Index %i is out of bounds!\n", id.index);
		return NULL;
	}

	if(s->offsets[index] >= s->first_free) {
		/* This entity is already dead. */
		DEBUG("Entity %lu is dead.", id.id);
		return NULL;
	}

	if(s->entities[s->offsets[index]].unique_id.id != id.id) {
		/* There is an entity here, but it's from a different generation */
		DEBUG("This entity is a thing of the past.\n");
		return NULL;
	}

	/* Everything's fluffy, lets return this one. */
	return &(s->entities[s->offsets[index]]);
}

/* Get the i-th global entity */
entity_t* get_entity_by_index(entity_storage_t* s, uint32_t i) {
	if(i >= s->first_free) {
		return NULL;
	}

	return &(s->entities[i]);
}

/* Delete an entity from the storage, freeing its slot */
void free_entity(entity_storage_t* s, entity_id_t id) {

	off_t index;
	off_t last_index;
	entity_t temp_entity;
	entity_t *o;
	entity_t *e = get_entity_from_storage_by_id(s, id);

	if(e == NULL) {
		if (id.id != INVALID_ID.id) {
			DEBUG("Attempted to free nonexistent entity with id %lu\n", id.id);
		}
		return;
	}

	/* Clean up the inner organs of this entity */
	destroy_entity(e);

	/* Get array index of this entity */
	index = s->offsets[id.index];

	/* Also get array index of the last occupied entity */
	last_index = s->first_free - 1;

	if(last_index > s->max) {
		DEBUG("Warning: Invalid first_free index in entity storage.\n");
	}

	o = &(s->entities[last_index]);

	/* Increment respawn-count of this entity */
	e->unique_id.reincarnation++;

	if(index != last_index) {
		/* Switch the two entities */
		temp_entity = *o;
		*o = *e;
		*e = temp_entity;

		/* Also switch their offset-table-pointers*/
		s->offsets[o->unique_id.index]=last_index;
		s->offsets[e->unique_id.index]=index;
	}

	/* And let the pointer to the first free entity now point at this one. */
	s->first_free--;

}

/* Free the storage and all of it's contents */
void cleanup_storage(entity_storage_t* s) {

	free(s->entities);

	free(s->offsets);

	free(s);
}
