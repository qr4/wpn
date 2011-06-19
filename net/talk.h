// 
// code by twist<at>nerd2nerd.org
//

#ifndef TALK_H
#define TALK_H

void init_talk();

void talk_log_lua_msg(unsigned int user_id, char* msg, int msg_len);

int talk_get_user_change_code_fd();

void talk_kill(); // do not use!!!!

#endif

