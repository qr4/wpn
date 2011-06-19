#ifndef  PLAYER_H
#define  PLAYER_H

#include "types.h"

extern player_data_t* players;
extern int n_players;

void new_player(unsigned int player_id);

#endif  /*PLAYER_H*/
