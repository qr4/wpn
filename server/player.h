#ifndef  PLAYER_H
#define  PLAYER_H

#include "types.h"

extern player_data_t* players;
extern int n_players;

/* Add a new player */
void new_player(unsigned int player_id);

/* Add all players known in the directory */
void add_all_known_players();

/* Check for incoming code from a player */
void player_check_code_updates();

#endif  /*PLAYER_H*/
