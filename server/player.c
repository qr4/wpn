#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "types.h"
#include "../net/config.h"
#include "debug.h"
#include "config.h"
#include "storages.h"
#include "base.h"

player_data_t* players=NULL;
int n_players=0;

/* Create a new player entry, because the networking code tells us to. */
void new_player(unsigned int player_id) {

	char* namefile;
	FILE* f;
	int i;
	entity_id_t planet_id, base_id;
	entity_t* b;

	for(i=0; i<n_players; i++) {
		if(players[i].player_id == player_id) {
			/* This player already exists, our work here is unneccessary */
			return;
		}
	}

	/* Allocate space for a new one */
	players = realloc(players, sizeof(player_data_t) * (n_players +1));
	if(!players) {
		ERROR("Allocating space for a new player failed!");
		exit(2);
	}

	/* Assign his ID */
	players[n_players].player_id = player_id;

	/* Find his userdir */
	asprintf(&namefile, USER_HOME_BY_ID "/%i/name", player_id);

	f = fopen(namefile, "r");
	if(!f) {
		ERROR("Can't determine name of user %u: reading %s failed: %s", player_id, namefile, strerror(errno));
		return;
	}

	/* Read username from one file */
	fscanf(f, "%as", &(players[n_players].name));

	/* If this fails for some reason, set an empty name */
	if(!players[n_players].name) {
		players[n_players].name = calloc(1,1);
	}

	/* Close the file */
	fclose(f);


	/* Find a nice, uninhabited planet */
	while(1) {
		/* TODO: What if all planets are full? */
		i=rand() % (planet_storage->first_free);
		if(planet_storage->entities[i].player_id == 0) {
			planet_id = planet_storage->entities[i].unique_id;
			break;
		}
	}

	DEBUG("New player '%s' connected with id %i\n", players[n_players].name, player_id);

	/* create a homebase for this player */
	base_id = init_base(base_storage, planet_id, config_get_int("initial_base_size"));
	players[n_players].homebase=base_id;

	b = get_entity_by_id(base_id);
	b->player_id = player_id;

	DEBUG("Created homebase %li for player %i.\n", base_id, player_id);

	n_players++;

	/* TODO: Send json update to the clients, informing about the new player */
}

