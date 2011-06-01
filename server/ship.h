#ifndef _SHIP_H
#define _SHIP_H

#include <stdint.h>
#include "entities.h"
#include "vector.h"
#include "types.h"
#include "entity_storage.h"

/* Create a new, empty ship */
entity_id_t init_ship(entity_storage_t* s, vector_t pos, uint8_t size);

#endif /* _SHIP_H */
