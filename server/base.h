#ifndef  BASE_H
#define  BASE_H
#include "types.h"
#include "entity_storage.h"

/* Create a new, empty base on some planet */
entity_id_t init_base(entity_storage_t* s, entity_id_t planet, uint8_t size);

#endif  /*BASE_H*/
