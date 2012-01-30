#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>

#include <pstr.h>

#include "buffer.h"
#include "json.h"
#include "sdl.h"
#include "types.h"

#define PORT "8080"


state_t state;
options_t options;
options_t options_old;

// puffer-eumel fuer net-data
struct dstr net_puffer;
struct dstr json_puffer;

// -1 = error -> close connection
// 0 = nur gewartet
// 1 = have new json-data
int checkInput(int net, buffer_t* b, int usleep_time) {

	static int need_empty_json_puffer = 1;

	// leeren wenn das letzte mal ein kompletter block gefunden wurde
	if (need_empty_json_puffer == 1) {
		need_empty_json_puffer = 0;
		dstr_clear(&json_puffer);
	}

	int ret;
	fd_set inputset;
	struct timeval timeout;

	FD_ZERO(&inputset);
	FD_SET(net, &inputset); // Add stdin to the (empty) set of input file descriptores

	timeout.tv_sec = 0;
	timeout.tv_usec = usleep_time;

	/* select returns 0 if timeout, 1 if input available, -1 if error. */
	ret = select(net + 1, &inputset, NULL, NULL, &timeout);

	if (ret == -1) {
		if(errno == EINTR) {
			return 0; // Unix is slightly stupid
		} else {
			// This is a real error
			fprintf(stderr, "select() produced error %d\n", errno);
			perror("select()");
			return -1;
		}
	}

	if (ret == 0) {
		// timer
		return 0;
	}

	// Input available
	if(FD_ISSET(net, &inputset)) {
		char buffer[40000];
		ssize_t len = recv(net, buffer, sizeof(buffer), 0);

		if ((len == -1) || (len == 0)) {
			printf("server hang up. Goodbye.\n");
			return -1;
		}

		dstr_append(&net_puffer, buffer, len);

		char* line;
		int line_len;
		while (dstr_read_line(&net_puffer, &line, &line_len) != -1) {
			if (line_len == 0) {
				b->size = dstr_len(&json_puffer);
				b->fill = dstr_len(&json_puffer);
				b->data = dstr_as_cstr(&json_puffer);
				need_empty_json_puffer = 1;
				return 1;
			} else if (len > 0) {
				dstr_append(&json_puffer, line, line_len);
			}
		}
	}

	// War wohl falscher Alarm ?!
	return 0;
}

void init_storage(storage_t *storage, size_t nmemb, size_t size) {
	size *= nmemb;
	storage->data = realloc(storage->data, size);
	if (storage->data == NULL) {
		fprintf(stderr, "No space to allocate storage of size %lu\n", size);
		exit(1);
	}
	storage->n = 0;
	storage->n_max = nmemb;
}

void allocStuff() {
	const size_t size = 1000;
	init_storage(&state.asteroids,     size,   sizeof(asteroid_t));
	init_storage(&state.bases,         size,   sizeof(base_t));
	init_storage(&state.explosions,    size,   sizeof(explosion_t));
	init_storage(&state.planets,       size,   sizeof(planet_t));
	init_storage(&state.ships,         size,   sizeof(ship_t));
	init_storage(&state.shots,         size,   sizeof(shot_t));
	init_storage(&state.players,       size,   sizeof(player_t));
}

// get sockaddr, IPv4 or IPv6:
// TODO... das steht doch schon in der network.c
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int connect2server(const char* server) {
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
		return -1;
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
		return -1;
	}

	return sockfd;
}

int reconnect2server(const char* server) {
	for (;;) {
		int fd = connect2server(server);
		if (fd != -1) {
			return fd;
		}
		sleep(1);
	}
}

void set_default_opts() {
	allocStuff();
	state.boundingbox.xmin = 0;
	state.boundingbox.xmax = 640;
	state.boundingbox.ymin = 0;
	state.boundingbox.ymax = 480;

	options.local_player = -1;
	options.display_x = 640;
	options.display_y = 480;
	options.mag = 200.0;
	options.zoom = 1.0/options.mag;
	options.offset_x = 0;
	options.offset_y = 0;
	options.influence_threshhold = 0.00000015;
	options.show_text_name = true;
	options.show_text_id = false;
	options.show_text_coords = false;
	options.show_influence = false;
}

int main(int argc, const char* argv[] ) {
	buffer_t buffer; // wir verwenden nur die datenstruktur = getBuffer();
	int ret;

	if (argc != 2 && argc != 3) {
		printf("%s <server-ip> [userid]\n", argv[0]);
		exit(EXIT_FAILURE);
	}


	dstr_malloc(&net_puffer);
	dstr_malloc(&json_puffer);
	int net = reconnect2server(argv[1]);

	set_default_opts();

	if(argc == 3) {
		options.local_player = atoi(argv[2]);
	}

	SDLinit();

	while(1) {
		options_old = options;
		checkSDLevent();

		ret = checkInput(net, &buffer, 100000);

		if (ret == -1) {
			close(net);
			net = reconnect2server(argv[1]);
		}

		if (ret == 1) {
			parseJson(&buffer);
		}

		SDLplot();

	}

	close(net);

	return 0;
}
