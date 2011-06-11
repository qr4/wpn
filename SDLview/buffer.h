#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

buffer_t* getBuffer();
void growBuffer(buffer_t* b);
void freeBuffer(buffer_t* b);
void clearBuffer(buffer_t* b);

#endif
