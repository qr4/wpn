#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>

#include "buffer.h"
#include "json.h"
#include "sdl.h"
#include "types.h"

#define PORT "8080"

asteroid_t* asteroids;
int n_asteroids, n_asteroids_max;

explosion_t* explosions;
int n_explosions, n_explosions_max;

planet_t* planets;
int n_planets, n_planets_max;

ship_t* ships;
int n_ships, n_ships_max;

shot_t* shots;
int n_shots, n_shots_max;

bbox_t boundingbox;

int checkInput(int net, buffer_t* b) {
	int ret;
	fd_set inputset;
	struct timeval timeout;

	static char lc = 0; // Char der in der letzten Runde gelesen wurde

	FD_ZERO(&inputset);
	FD_SET(net, &inputset); // Add stdin to the (empty) set of input file descriptores

	timeout.tv_sec = 0;
	timeout.tv_usec = 100;

	/* select returns 0 if timeout, 1 if input available, -1 if error. */
	ret = select(net + 1, &inputset, NULL, NULL, &timeout);

	if(ret == 1) {
		// Input available
		if(FD_ISSET(net, &inputset)) {

      // TODO
      // ich hasse den code!!!! armes netzwerk!
      char c[1];
      ssize_t len = recv(net, c, sizeof(c), 0);

      if (len == 0) {
        // handle server hang-up
        fprintf(stderr, "server hang up. Goodbye.\n");
        return -1;
      }

			if(b->fill >= b->size - 1) {
				growBuffer(b);
			}
			b->data[b->fill] = c[0];
			b->fill++;
			if(lc == '\n' && c[0] == '\n') {
				// Wir haben eine Leerzeile, sprich ein JSON-Objekt sollte zu Ende sein
				return 2;
			}
			lc = c[0];
			return 1;
		}
		// War wohl falscher Alarm
		return 0;
	} else if(ret == -1) {
		// Error
		if(errno == EINTR) {
			// Unix is slightly stupid
			return 0;
		} else {
			// This is a real error
			fprintf(stderr, "select() produced error %d\n", errno);
      perror("select()");
			return -1;
		}
	} else {
		// Timeout
		return 0;
	}
}

void allocStuff() {
	asteroids = malloc(1000 * sizeof(asteroid_t));
	if(!asteroids) {
		fprintf(stderr, "No space left for asteroids\n");
		exit(1);
	}
	n_asteroids = 0;
	n_asteroids_max = 1000;

	explosions = malloc(1000 * sizeof(explosion_t));
	if(!explosions) {
		fprintf(stderr, "No space left for explosions\n");
		exit(1);
	}
	n_explosions = 0;
	n_explosions_max = 1000;

	planets = malloc(1000 * sizeof(planet_t));
	if(!planets) {
		fprintf(stderr, "No space left for planets\n");
		exit(1);
	}
	n_planets = 0;
	n_planets_max = 1000;

	ships = malloc(1000 * sizeof(ship_t));
	if(!ships) {
		fprintf(stderr, "No space left for ships\n");
		exit(1);
	}
	n_ships = 0;
	n_ships_max = 1000;

	shots = malloc(1000 * sizeof(shot_t));
	if(!shots) {
		fprintf(stderr, "No space left for shots\n");
		exit(1);
	}
	n_shots = 0;
	n_shots_max = 1000;
}

// get sockaddr, IPv4 or IPv6:
// TODO... das steht doch schon in der network.c
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int connect2server(char* server) {

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;

  bzero(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  printf("connect to server %s\n", server);
  rv = getaddrinfo(server, PORT, &hints, &servinfo);

  if (rv != 0) {
    printf("getaddrinfo: %s\n", gai_strerror(rv));
    freeaddrinfo(servinfo); // all done with this structure
    exit(EXIT_FAILURE);
  }

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("connect_to_server client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("connect_to_server client: connect");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL) {
    printf("connect_to_server client: failed to connect\n");
    exit(EXIT_FAILURE);
  }

  return sockfd;
}

int main(int argc, const char* argv[] ) {
	buffer_t* buffer = getBuffer();
	int ret;

  if (argc != 2) {
    printf("%s <server-ip>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int net = connect2server(argv[1]);

	allocStuff();

	boundingbox.xmin = 0;
	boundingbox.xmax = 640;
	boundingbox.ymin = 0;
	boundingbox.ymax = 480;

	SDLinit();

	while(1) {
		checkSDLevent();

		SDLplot();

		ret = 1;
		while(ret == 1) {
			ret = checkInput(net, buffer);
		}

		if(ret < 0) {
			fprintf(stderr, "I guess we better quit\n");
			exit(1);
		}

		if(ret == 0) {
			usleep(1000);
		}

		if(ret == 2) {
			//fprintf(stdout, "Read %d chars to find an empty line\n", buffer->fill);
			parseJson(buffer);
			clearBuffer(buffer);
		}
	}

  close(net);

	return 0;
}
