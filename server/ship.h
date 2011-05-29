#ifndef _SHIP_H
#define _SHIP_H

#include <stdint.h>
#include "entities.h"
#include "vector.h"

/* Create a new, empty ship */
extern void init_ship(entity_t* e, vector_t pos, uint8_t size);

#endif /* _SHIP_H */
