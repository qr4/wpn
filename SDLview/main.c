#include <errno.h>
#include <unistd.h>

#include "buffer.h"
#include "json.h"
#include "sdl.h"
#include "types.h"

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

int checkInput(buffer_t* b) {
	int ret;
	fd_set inputset;
	struct timeval timeout;

	static char lc = 0; // Char der in der letzten Runde gelesen wurde

	FD_ZERO(&inputset);
	FD_SET(0, &inputset); // Add stdin to the (empty) set of input file descriptores

	timeout.tv_sec = 0;
	timeout.tv_usec = 100;

	/* select returns 0 if timeout, 1 if input available, -1 if error. */
	ret = select(FD_SETSIZE, &inputset, NULL, NULL, &timeout);

	if(ret == 1) {
		// Input available
		if(FD_ISSET(0, &inputset)) {
			// And it's acutally on stdin \o/
			char c = fgetc(stdin);
			if(c == EOF) {
				fprintf(stderr, "Stdin was closed. Goodbye.\n");
				usleep(3e6);
				return -1;
			}
			if(b->fill >= b->size - 1) {
				growBuffer(b);
			}
			b->data[b->fill] = c;
			b->fill++;
			if(lc == '\n' && c == '\n') {
				// Wir haben eine Leerzeile, sprich ein JSON-Objekt sollte zu Ende sein
				return 2;
			}
			lc = c;
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

int main() {
	buffer_t* buffer = getBuffer();
	int ret;

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
			ret = checkInput(buffer);
		}

		if(ret < 0) {
			fprintf(stderr, "I guess we better quit\n");
			exit(1);
		}

		if(ret == 0) {
			usleep(10000);
		}

		if(ret == 2) {
			fprintf(stdout, "Read %d chars to find an empty line\n", buffer->fill);
			parseJson(buffer);
			clearBuffer(buffer);
		}
	}

	return 0;
}
