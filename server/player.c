#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <poll.h>
#include <unistd.h>
#include <lauxlib.h>
#include "types.h"
#include "../net/config.h"
#include "../net/talk.h"
#include "debug.h"
#include "config.h"
#include "json_output.h"
#include "luastate.h"
#include "player.h"
#include "storages.h"
#include "base.h"

player_data_t* players=NULL;
int n_players=0;

/* Create a new player entry, because the networking code tells us to. */
void new_player(unsigned int player_id) {

	char* namefile;
	FILE* f;

	/* if this player already exists, our work here is unneccessary */
	if(find_player(player_id)) {
		return;
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

	DEBUG("New player '%s' connected with id %i\n", players[n_players].name, player_id);

	/* create a homebase for this player */
	create_homebase(&(players[n_players]));

	n_players++;

	/* Send json update to the clients, informing about the new player */
	player_updates_to_network();
}

/* Create a new homebase for this player (either because he just joined, or
 * just died) */
void create_homebase(player_data_t* player) {
	entity_id_t base_id;
	entity_id_t planet_id;
	entity_t* b;
	int i;

	if(get_entity_by_id(player->homebase)) {
		ERROR("Player %u already has a homebase!\n", player->player_id);
		return;
	}

	/* Find a nice, uninhabited planet */
	while(1) {
		/* TODO: What if all planets are full? */
		i=rand() % (planet_storage->first_free);
		if(planet_storage->entities[i].player_id == 0) {
			planet_id = planet_storage->entities[i].unique_id;
			break;
		}
	}

	base_id = init_base(base_storage, planet_id, config_get_int("initial_base_size"));
	player->homebase=base_id;

	b = get_entity_by_id(base_id);
	b->player_id = player->player_id;

	DEBUG("Created homebase %li for player %i.\n", base_id, player->player_id);
}

/* Scan through the player-by-id directory and add all players marked there */
/* This can for example be done at restart of a round, to supply all players with
 * a new homebase */
void add_all_known_players() {
	DIR* playerdir;
	struct dirent* direntry;
	int number;

	playerdir = opendir(USER_HOME_BY_ID);
	if(!playerdir) {
		ERROR("Unable to open player directory: %s\nNot adding players.", strerror(errno));
		return;
	}

	/* add all players */
	while(1) {
		direntry = readdir(playerdir);
		if(!direntry)
			break;

		/* Don't add direntrys that don't resolve to a number */
		number = atoi(direntry->d_name);
		if(number)
			new_player(number);
	}

	closedir(playerdir);
}

/* Look if the network code provides us with some new shiny player data */
void player_check_code_updates(long usec_wait) {
  fd_set rfds, rfds_tmp;
  struct timeval tv;
	char* errortext;
  int ret;

  // auf talk_get_user_change_code_fd() hoeren
  FD_ZERO(&rfds);
  FD_SET(talk_get_user_change_code_fd(), &rfds);

  // timeout
  tv.tv_sec = 0;
  tv.tv_usec = usec_wait;

  int max_fd = talk_get_user_change_code_fd();

  for(;;) {
    rfds_tmp = rfds;
    ret = select(max_fd + 1, &rfds_tmp, NULL, NULL, &tv);

    if (ret == -1) { perror("select player_check_code_updates"); exit(EXIT_FAILURE); }

    if (ret) {
      // fd jammert rum
      for (int fd = 0; fd <= max_fd; ++fd) {
        if (FD_ISSET(fd, &rfds_tmp)) {

          int data[100];

          ssize_t read_len = read(fd, data, sizeof(data));
          if (read_len == -1) { perror("read player_check_code_updates"); exit(EXIT_FAILURE); }
          if (read_len == 0) { printf("client tot...\n"); exit(EXIT_FAILURE); }
          if ((read_len & 0x3) != 0) { printf("oops. we lost an update. boeses socket, boeses!\n"); exit(EXIT_FAILURE); }

          for (int i = 0; i < read_len / sizeof(unsigned int); ++i) {
            unsigned int user = data[i];
            entity_id_t base;
            entity_t* ebase;
            int j;
            char* lua_source_file;

            DEBUG("Update for player %d", user);

            /* Make sure this player exists */
            new_player(user);

            for(j = 0; j < n_players; j++) {
              if(players[j].player_id == user) {
                break;
              }
            }

            /* Get the location we're reading code from */
            asprintf(&lua_source_file, USER_HOME_BY_ID "/%i/current", user);

            /* TODO: Make certain a player's homebase never gets destroyed (or replace it) */
            base = players[j].homebase;
            ebase = get_entity_by_id(base);

            /* Evaluate the code in the context of the player's homebase */
            lua_active_entity = base;

            if(!(ebase->lua)) {
              ERROR("Player %u's homebase lua state is dead. This shouldn't happen.\n", user);
              return;
            }
            DEBUG("Executing %s in the context of entity %lu\n", lua_source_file, base.id);
            if(luaL_dofile(ebase->lua, lua_source_file)) {
							DEBUG("Execution failed.\n");
							errortext = lua_tostring(ebase->lua, -1);
							talk_log_lua_msg(user, errortext, strlen(errortext));
							lua_pop(ebase->lua, 1);
						}

            free(lua_source_file);
          }
        }
      }
    } else {
      // timer
      return;
    }
  }
}

player_data_t* find_player(unsigned int player_id) {
	int i;
	for(i=0; i<n_players; i++) {
		if(players[i].player_id == player_id) {
			return &players[i];
		}
	}

	return NULL;
}
