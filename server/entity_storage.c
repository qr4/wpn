#include "entity_storage.h"
#include "debug.h"

/* Create a new, empty entity storage, with the specified starting size and
 * type bitmask added to the entities' ids */
entity_storage_t* init_entity_storage(const uint32_t n_entries, const uint16_t type) {

	int i;

	/* Allocate space for the storage itself */
	entity_storage_t* s = malloc(sizeof(entity_storage_t));
	
	/* If no default size has been specified, just assume... something */
	if(n_entries == 0) {
		s->max = 32;
	} else {
		s->max = n_entries;
	}

	/* Allocate space for the entries */
	s->entities=malloc(sizeof(entity_t) * s->max);
	s->offsets=malloc(sizeof(uint32_t) * s->max);

	/* Initialize offset indices in a consistent way */
	for(i=0;i<s->max;i++)
	{
		s->offsets[i]=i;
		s->entities[i].unique_id = (entity_id_t){{.index=i, .reincarnation=0, .type=type}};
	}
	
	s->first_free = 0;

	return s;
}

/* Allocate a new entity from the storage pool */
entity_id_t alloc_entity(entity_storage_t* s) {

	entity_t* retval;
	int i;

	/* Sanity check: Full? */
	if(s->first_free >= s->max-1) {
		//fprintf(stderr, "Entity cap reached!\n");
		//return NULL;

		s->max *= 2;
		s->offsets = realloc(s->offsets, sizeof(uint32_t) * s->max);
		s->entities = realloc(s->entities, sizeof(entity_t) * s->max);

		for(i=s->first_free; i<s->max; i++) {
			s->offsets[i]=i;
			s->entities[i].unique_id.id=i;
		}
		

	}

	retval = &(s->entities[s->first_free]);

	s->first_free++;

	return retval->unique_id;
}

/* Get an entity from its unique id. If no such entity exists, returns NULL */
entity_t* get_entity_from_storage_by_id(entity_storage_t* s, entity_id_t id) {

	uint32_t index = id.index;

	if(index >= s->max) {
		/* Entity out of bounds for this storage. Well, what do we care. */
		return NULL;
	}

	if(s->offsets[index] >= s->first_free) {
		/* This entity is already dead. */
		return NULL;
	}

	if(s->entities[s->offsets[index]].unique_id.id != id.id) {
		/* There is an entity here, but it's from a different generation */
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
	off_t temp;
	entity_t temp_entity;
	entity_t *o;
	entity_t *e = get_entity_from_storage_by_id(s, id);

	if(e == NULL) {
		DEBUG("Attempted to free nonexistent entity with id %i\n", id);
		return;
	}

	/* Clean up the inner organs of this entity */
	destroy_entity(e);

	/* Get array index of this entity */
	index = id.index;

	/* Also get array index of the last occupied entity */
	last_index = s->first_free - 1;

	if(last_index > s->max) {
		DEBUG("Warning: Invalid first_free index in entity storage.\n");
	}

	o = &(s->entities[s->offsets[last_index]]);

	/* Increment respawn-count of this entity */
	e->unique_id.reincarnation++;

	if(index != last_index) {
		/* Switch the two entities */
		temp_entity = *o;
		*o = *e;
		*e = temp_entity;

		/* Also switch their offset-table-pointers*/
		temp = s->offsets[index];
		s->offsets[index]=s->offsets[last_index];
		s->offsets[last_index]=temp;
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
