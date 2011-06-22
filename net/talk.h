// 
// code by twist<at>nerd2nerd.org
//

#ifndef TALK_H
#define TALK_H

// init. bitte 1x aufrufen
void init_talk();

// fuer papa: code-nachrichten verschicken und lesen via pipe
// menupunkt 1
int talk_get_user_code_fd();  // receive
void talk_set_user_code_reply_msg(unsigned int user_id, char* msg, int msg_len); // send

// auf den hoert papa und bekommt mit, dass $user neuen code hochgeladen hat
// menupunkt 2
int talk_get_user_code_upload_fd(); // receive

// fuer papa: verschicken von lua-ausgaben des laufenden programms
// menupunkt 3
void talk_log_lua_msg(unsigned int user_id, char* msg, int msg_len);  //send

void talk_kill(); // do not use!!!!

#endif

