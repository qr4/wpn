#ifndef _STATUSBOARD_H
#define _STATUSBOARD_H

#include "../ClientLib/consumer.h"
#include "../ClientLib/json.h"
#include "../ClientLib/types.h"

typedef struct score_t score_t;

struct score_t {
	player_t player;
	int planets;
	int bases;
	int ships;
	int score;
	int rank;
};

void init_storage(storage_t *storage, size_t nmemb, size_t size);
json_int_t follow_ship = 0;

#endif
