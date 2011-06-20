#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <lauxlib.h>
#include "types.h"
#include "../net/config.h"
#include "../net/talk.h"
#include "debug.h"
#include "config.h"
#include "luastate.h"
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

/* Look if the network code provides us with some new shiny player data */
void player_check_code_updates() {
	int fd = talk_get_user_change_code_fd();
	struct pollfd poll_data;
	unsigned int user;
	entity_id_t base;
	entity_t* ebase;
	int i;
	char* lua_source_file;

	poll_data.fd = fd;
	poll_data.events=POLLIN;

	/* Check if any data is present */
	if(poll(&poll_data, 1, 1)) {

		/* Read the id of a user */
		read(fd, &user, sizeof(unsigned int));

		DEBUG("Update for player %u", user);

		/* Make sure this player exists */
		new_player(user);

		for(i=0; i<n_players; i++) {
			if(players[i].player_id == user) {
				break;
			}
		}

		/* Get the location we're reading code from */
		asprintf(&lua_source_file, USER_HOME_BY_ID "/%i/current", user);

		/* TODO: Make certain a player's homebase never gets destroyed (or replace it) */
		base = players[i].homebase;
		ebase = get_entity_by_id(base);

		/* Evaluate the code in the context of the player's homebase */
		lua_active_entity = base;

		if(!(ebase->lua)) {
			ERROR("Player %u's homebase lua state is dead. This shouldn't happen.\n", user);
			return;
		}
		DEBUG("Executing %s in the context of entity %lu\n", lua_source_file, base.id);
		luaL_dofile(ebase->lua, lua_source_file);
	}
}
