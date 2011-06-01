#include <stdlib.h>
#include <stdint.h>
#include "debug.h"

typedef struct id_map_t id_map_t;

struct id_map_t {
	uint16_t reincarnation;
	uint16_t id;
};

uint16_t *offset = NULL;
id_map_t *ship = NULL;
uint16_t ships = 0;
uint16_t max_ships = 0;

void init_id_map(const uint16_t n) {
	uint16_t i;
	max_ships = n;
	ships = 0;

	offset = (uint16_t *) malloc (sizeof(uint16_t) * n);
	ship   = (id_map_t *) malloc (sizeof(id_map_t) * n);

	for (i = 0; i < n; i++) {
		offset[i] = i;
		ship[i] = (id_map_t) {0, i};
	}
}
	
id_map_t *new_ship() {
	uint16_t i;
	
	DEBUG("Adding ship: %d / %d\n", ships, max_ships);
	
	if (ships == max_ships) {
		if (max_ships >= 1<<15) {
			return NULL;
		}

		max_ships *= 2;

		DEBUG("Not enough space, doubling space to %d\n", max_ships);

		offset = (uint16_t *) realloc (offset, sizeof(uint16_t) * max_ships);
		ship   = (id_map_t *) realloc (ship,   sizeof(id_map_t) * max_ships);

		for (i = ships; i < max_ships; i++) {
			offset[i] = i;
			ship[i] = (id_map_t) {0, i};
		}
		
	}

	return &ship[ships++];
}

id_map_t *get_ship(id_map_t id) {

	if (id.id >= max_ships) {
		DEBUG("id impossible high (%d)\n", id.id);
		return NULL;
	}

	if (ship[offset[id.id]].reincarnation == id.reincarnation && ship[offset[id.id]].id == id.id) {
		return &(ship[offset[id.id]]);
	} else {
		return NULL;
	}
}

void del_ship(id_map_t id) {
	uint16_t t_offset;
	id_map_t t_id_map;
	id_map_t *last, *to_remove;

	if (ships == 0) {
		DEBUG("ships == 0\n");
		return;
	}
	if ((to_remove = get_ship(id)) == NULL) {
		DEBUG("to_remove == 0\n");
		return;
	}
	last = &ship[ships-1];

	to_remove->reincarnation++;

	DEBUG("Removing id %d\n", to_remove->id);
	if (last != to_remove) {
		t_offset = offset[last->id];
		offset[last->id] = offset[to_remove->id];
		offset[to_remove->id] = t_offset;

		t_id_map = *last;
		*last = *to_remove;
		*to_remove = t_id_map;
	}

	ships--;
}

void free_ships() {
	if (offset) {
		free(offset);
		offset = NULL;
	}
	if (ship) {
		free(ship);
		offset = NULL;
	}
}

void print_mapping() {
	uint16_t i;

	DEBUG("ID  |");
	for (i = 0; i < max_ships; i++) {
		DEBUG("%9d |", i);
	}
	DEBUG("\nOFF |");
	for (i = 0; i < max_ships; i++) {
		DEBUG("%9d |", offset[i]);
	}
	DEBUG("\nSHI |");
	for (i = 0; i < ships; i++) {
		DEBUG("%4d %4d |", ship[i].reincarnation, ship[i].id);
	}
	DEBUG("\b#");
	for (; i < max_ships; i++) {
		DEBUG("%4d %4d |", ship[i].reincarnation, ship[i].id);
	}
	DEBUG("\n");
}

#ifdef MAPPING_MAIN
#define N 32768
int main(int argc, char *argv[]) {
	uint16_t i;
	id_map_t tmp[N];
	init_id_map(2);

	for (i = 0; i < N; i++) {
		tmp[i] = *(new_ship());
		DEBUG("Got ship reincarnated %d times witd id %d\n", get_ship(tmp[i])->reincarnation, get_ship(tmp[i])->id);
	}

	for (i = 0; i < N; i++) {
		print_mapping();
		del_ship(tmp[i]);
	}
	print_mapping();

	free_ships();

	return 0;
}
#endif
