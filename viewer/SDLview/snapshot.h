#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "types.h"

void        snapshot_update(snapshot_t *snapshot);
snapshot_t *snapshot_getcurrent();
void        snapshot_getcurrentcopy(snapshot_t *snapshot);
void        snapshot_release(snapshot_t *snapshot);

#endif  /*SNAPSHOT_H*/
