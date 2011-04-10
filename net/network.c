
//
// code by twist<at>nerd2nerd.org
//

#include "network.h"

#include <logging.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <unistd.h>

#include <string.h>


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// bind to socket: -1 = geht nicht
int network_bind(char* port, int max_connection) {
  int listener;     // listening socket descriptor
  int yes = 1;      // for setsockopt() SO_REUSEADDR
  int rv;

  struct addrinfo hints, *ai, *p;

  // get us a socket and bind it
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
    log_msg("getaddrinfo %s", gai_strerror(rv));
    goto err;
  }

  for(p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) {
      continue;
    }

    // lose the pesky "address already in use" error message
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener);
      continue;
    }

    break;
  }

  // if we got here, it means we didn't get bound
  if (p == NULL) {
    log_msg("failed to bind server - alles belegt???");
    goto err;
  }

  // listen
  if (listen(listener, max_connection) == -1) {
    log_perror("listen");
    close(listener);
    goto err;
  }

  freeaddrinfo(ai); // all done with this

  return listener;

err:
  freeaddrinfo(ai); // all done with this
  return -1;
}



