#ifndef  STORAGES_H
#define  STORAGES_H

#include "types.h"
#include "entity_storage.h"

extern entity_storage_t* ship_storage;
extern entity_storage_t* base_storage;
extern entity_storage_t* asteroid_storage;
extern entity_storage_t* planet_storage;
extern entity_storage_t* cluster_storage;

void init_all_storages();
void free_all_storages();

/* Get an entity of any type */
entity_t* get_entity_by_id(entity_id_t id);

#endif  /*STORAGES_H*/
