#ifndef  PLAYER_H
#define  PLAYER_H

#include "types.h"

extern player_data_t* players;
extern int n_players;

/* Add a new player */
void new_player(unsigned int player_id);

void create_homebase(player_data_t* player);

/* Add all players known in the directory */
void add_all_known_players();

/* Load the most recent versions of each player's lua code */
void evaluate_all_player_code();

/* Check for incoming code from a player */
void player_check_code_updates();

/* Load the initial base code of a player */
void evaluate_player_base_code(player_data_t* p, entity_id_t base);

/* Find the data structure belonging to a player */
player_data_t* find_player(unsigned int player_id);

#endif  /*PLAYER_H*/
