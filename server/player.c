#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <poll.h>
#include <unistd.h>
#include <lauxlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "types.h"
#include "../net/config.h"
#include "../net/talk.h"
#include "../net/dispatch.h"
#include "debug.h"
#include "luafuncs.h"
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
	if(fscanf(f, "%as", &(players[n_players].name)));

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
			planet_storage->entities[i].player_id = player->player_id;
			break;
		}
	}

	base_id = init_base(base_storage, planet_id, config_get_int("initial_base_size"));
	player->homebase=base_id;

	b = get_entity_by_id(base_id);
	b->player_id = player->player_id;

	DEBUG("Created homebase %lu for player %u.\n", base_id.id, player->player_id);

	/* Lua update this */
	send_base_update(b);
	send_planet_update(get_entity_by_id(planet_id));
}

/* Scan through the player-by-id directory and add all players marked there */
/* This can for example be done at restart of a round, to supply all players with
 * a new homebase */
void add_all_known_players() {
	DIR* playerdir;
	struct dirent* direntry;
	int number;
	struct stat codestat;
	char* codefile;

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
		if(!number)
			continue;

		/* Don't add direntries that don't contain code. */
		asprintf(&codefile, USER_HOME_BY_ID "/%i/current", number);
		if(stat(codefile, &codestat) || codestat.st_size == 0) {
			free(codefile);
			continue;
		}
		free(codefile);

		new_player(number);

	}

	closedir(playerdir);
}

static struct pipe_com user_code;

/* Look if the network code provides us with some new shiny player data */
void player_check_code_updates(long usec_wait) {
  fd_set rfds, rfds_tmp;
  struct timeval tv;
	char* errortext, *returntext;
  int ret;
	int n,i;

  // auf talk_get_user_change_code_fd() hoeren
  FD_ZERO(&rfds);
  FD_SET(talk_get_user_code_fd(), &rfds);
  FD_SET(talk_get_user_code_upload_fd(), &rfds);

  // timeout
  tv.tv_sec = 0;
  tv.tv_usec = usec_wait;

#ifndef MAX
# define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
  int max_fd = MAX(talk_get_user_code_fd(), talk_get_user_code_upload_fd());

  for(;;) {
    rfds_tmp = rfds;
    ret = select(max_fd + 1, &rfds_tmp, NULL, NULL, &tv);

    if (ret == -1) { perror("select player_check_code_updates"); exit(EXIT_FAILURE); }

    if (ret) {
      // fd jammert rum
      for (int fd = 0; fd <= max_fd; ++fd) {
        if (FD_ISSET(fd, &rfds_tmp)) {
          if (fd == talk_get_user_code_fd()) {
            // ein user hat was per hand eingegeben
            char* data;
            int data_len;
            int ret = dispatch(&user_code, fd, &data, &data_len);
            if (ret == -1) {
              ERROR("dispatch sagt -1\n");
              exit(EXIT_FAILURE);
            }

            if (data_len == 0) break;  // keine daten -> nix an lua senden

            int user = user_code.header.head.id;
            int j;
            entity_t* ebase;
            DEBUG("Code from user %d", user);

            new_player(user);

            for (j = 0; j < n_players; ++j) {
              if (players[j].player_id == user) {
                break;
              }
            }

            lua_active_entity = players[j].homebase;
            ebase = get_entity_by_id(players[j].homebase);

						/* Set the execution time limit */
						lua_sethook(ebase->lua, time_exceeded_hook, LUA_MASKCOUNT, config_get_int("lua_max_cycles"));

						/* Execute the code */
            if (luaL_dostring (ebase->lua, data)) {
              DEBUG("Execution failed.\n");
              errortext = (char*) lua_tostring(ebase->lua, -1);
              talk_set_user_code_reply_msg(user, errortext, strlen(errortext));
              lua_pop(ebase->lua, 1);

							if(do_reset) {
								lua_close(ebase->lua);
								ebase->lua=NULL;
								init_ship_computer(ebase);
								do_reset = 0;
							}
            } else {

							/* Send return values to the client, as strings */
							n = lua_gettop(ebase->lua);
							for(i=0; i<n; i++) {
								returntext = lua_tostring(ebase->lua,i+1);
								if(returntext) {
									talk_set_user_code_reply_msg(user, returntext, strlen(returntext));
                  talk_set_user_code_reply_msg(user, "\n", 1);
								} else {
									talk_set_user_code_reply_msg(user, "(nil)\n", 6);
								}
							}
              lua_pop(ebase->lua, n);
						}

            // request send prompt
            talk_set_user_code_reply_msg(user, "400 lua: ", 9);

          } else if (fd == talk_get_user_code_upload_fd()) {
            // der upload new file-fd hat was fuer uns
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

              base = players[j].homebase;
              ebase = get_entity_by_id(base);

							/* Set the execution time limit */
							lua_sethook(ebase->lua, time_exceeded_hook, LUA_MASKCOUNT, config_get_int("lua_max_cycles"));
              /* Evaluate the code in the context of the player's homebase */
              lua_active_entity = base;

              if(!(ebase->lua)) {
                ERROR("Player %u's homebase lua state is dead. This shouldn't happen.\n", user);
                return;
              }
              DEBUG("Executing %s in the context of entity %lu\n", lua_source_file, base.id);

              if(luaL_dofile(ebase->lua, lua_source_file)) {
                DEBUG("Execution failed.\n");
                errortext = (char*) lua_tostring(ebase->lua, -1);
                talk_log_lua_msg(user, errortext, strlen(errortext));
                lua_pop(ebase->lua, 1);

								if(do_reset) {
									lua_close(ebase->lua);
									ebase->lua=NULL;
									init_ship_computer(ebase);
									do_reset = 0;
								}
              }

              free(lua_source_file);
            }
          } else {
            // hu?! welcher fd war das denn?
            ERROR("unknown fd is talking?!??!\n");
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


/* When starting up, evaluate all "old" lua code we got flying around */
void evaluate_all_player_code() {
	char* lua_source_file;
	entity_id_t base;
	entity_t* ebase;
	char* errortext;

	for(int i=0; i<n_players; i++) {
		/* Get the location we're reading code from */
		asprintf(&lua_source_file, USER_HOME_BY_ID "/%i/current", players[i].player_id);

		base = players[i].homebase;
		ebase = get_entity_by_id(base);

		if(!ebase) {
			ERROR("Player %u has got no homebase.\n", players[i].player_id);
			continue;
		}

		/* Evaluate the code in the context of the player's homebase */
		lua_active_entity = base;

		if(!(ebase->lua)) {
			ERROR("Player %u's homebase lua state is dead. This shouldn't happen.\n", players[i].player_id);
			free(lua_source_file);
			continue;
		}

		/* Set the execution time limit */
		lua_sethook(ebase->lua, time_exceeded_hook, LUA_MASKCOUNT, config_get_int("lua_max_cycles"));

		DEBUG("Executing %s in the context of entity %lu\n", lua_source_file, base.id);
		if(luaL_dofile(ebase->lua, lua_source_file)) {
			DEBUG("Execution failed.\n");
			errortext = (char*) lua_tostring(ebase->lua, -1);
			talk_log_lua_msg(players[i].player_id, errortext, strlen(errortext));
			lua_pop(ebase->lua, 1);

			if(do_reset) {
				lua_close(ebase->lua);
				ebase->lua=NULL;
				init_ship_computer(ebase);
				do_reset = 0;
			}
		}

		free(lua_source_file);

	}
}
