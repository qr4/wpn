
//
// code by twist<at>nerd2nerd.org
//

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <arpa/inet.h>

void *get_in_addr(struct sockaddr *sa);
int network_bind(char* port, int max_connection);

#endif


