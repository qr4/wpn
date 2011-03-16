#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <errno.h>

#include "globals.h"
#include "screen.h"
#include "route.h"
#include "json_output.h"

/* TODO:
 *
 * a global scene for easy rerender
 * fast image switch
 */

SDL_Surface *screen;

void sdl_setup() {
	const SDL_VideoInfo *info;
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
		exit(1);
	}

	info = SDL_GetVideoInfo();
	GLOBALS.DISPLAY_WIDTH  = info->current_w;
	GLOBALS.DISPLAY_HEIGHT = info->current_h;

	atexit(SDL_Quit);
}

void sdl_screen_init() {
	if (GLOBALS.FULLSCREEN) {
		screen = SDL_SetVideoMode(GLOBALS.DISPLAY_WIDTH, GLOBALS.DISPLAY_HEIGHT, 0, SDL_FULLSCREEN);
	} else {
		screen = SDL_SetVideoMode(GLOBALS.WIDTH, GLOBALS.HEIGHT, 0, 0);
	}
}

void sdl_init() {
	sdl_setup();
	sdl_screen_init();

	if (screen == NULL) {
		fprintf(stderr, "SDL_SetVideoMode() failed: %s\n", SDL_GetError());
		exit(1);
	}
}

void draw_line(vector_t* p1, vector_t* p2) {
	aalineRGBA(screen, p1->x, p1->y, p2->x, p2->y, 255, 255, 255, 255);
}

void draw_blob(cluster_t* p) {
	if(p->safety_radius > 0) {
		aacircleRGBA(screen, p->center.x, p->center.y, 2, 255, 255, 255, 255);
		aacircleRGBA(screen, p->center.x, p->center.y, p->safety_radius, 64, 64, 64, 255);
	}
}

void draw_clusters(cluster_t* clusters, int n) {
	int x, y;

	for (y = 0; y < n; y++) {
		for (x = 0; x < n; x++) {
			draw_blob(&clusters[y*n + x]);
		}
	}
}

void draw_grid(cluster_t *clusters, int n) {
	int x, y;

	for (y = 0; y < n - 1; y++) {
		for (x = 0; x < n - 1; x++) {
			draw_line(&(clusters[y*n + x].center), &(clusters[y*n + x + 1].center));
			draw_line(&(clusters[y*n + x].center), &(clusters[(y + 1)*n + x].center));
		}
		draw_line(&(clusters[y*n + x].center), &(clusters[(y + 1)*n + x].center));
	}
	for (x = 0; x < n - 1; x++) {
		draw_line(&(clusters[y*n + x].center), &(clusters[y*n + x + 1].center));
	}
}

void draw_route(waypoint_t* route, int r, int g, int b, int with_blobs) {
	if (route == NULL) return;
	waypoint_t* t = route;
	while (t->next != NULL) {
		if (with_blobs) aacircleRGBA(screen, t->point.x, t->point.y, 2, r, g, b, 255);
		aalineRGBA(screen, t->point.x, t->point.y, t->next->point.x, t->next->point.y, r, g, b, 128);
		t = t->next;
	}
	aacircleRGBA(screen, t->point.x, t->point.y, 2, r, g, g, 255);
}

void squirl(cluster_t *clusters, int n) {
	int i, j;

	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			float x = clusters[i*n + j].center.x;
			float y = clusters[i*n + j].center.y;
			float r = hypotf(x-GLOBALS.WIDTH / 2, y - GLOBALS.HEIGHT/2);
			float phi = atan2f(y-GLOBALS.HEIGHT / 2, x - GLOBALS.WIDTH/2);
			float angle = (1e-3 * r - 0.5);
			phi += angle;
			x = GLOBALS.WIDTH/2 + r*cos(phi);
			y = GLOBALS.HEIGHT/2 + r * sin(phi);
			clusters[i*n + j].center.x = x;
			clusters[i*n + j].center.y = y;
		}
	}
}

void randomize(cluster_t *clusters, int n, float fac) {
	int i, j;

	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			clusters[i*n + j].center.x += (((float) rand() / RAND_MAX) - 0.5) * 2 * fac;
			clusters[i*n + j].center.y += (((float) rand() / RAND_MAX) - 0.5) * 2 * fac;
		}
	}
}

void set(cluster_t *clusters, int n, float safety_radius) {
	int x, y;

	for (y = 0; y < n; y++) {
		for (x = 0; x < n; x++) {
			clusters[y*n + x].center.x = GLOBALS.WIDTH  * x / (n - 1);
			clusters[y*n + x].center.y = GLOBALS.HEIGHT * y / (n - 1);
			if(x == 0 || y == 0 || x == n-1 || y == n-1) {
				// Not a real point, just used to extend the grid to the window size
				clusters[y*n + x].safety_radius = 0;
			} else {
				clusters[y*n + x].safety_radius = (y*n+x)%5 ? safety_radius : safety_radius/2;
			}
		}
	}

	squirl(clusters, n);
}

int main_loop() {
	int quit = 0;
	int n = 16;
	int view_grid = 0;
	int view_points  = 1;
	int view_waypoints = 1;
	int view_smoothed = 1;
	int view_json = 0;
	int safety_radius = GLOBALS.HEIGHT / (n + 1) / 5;
	float fac = GLOBALS.WIDTH / (n + 1) * 0.3;
	SDL_Event e;
	vector_t start = {0, 0};
	vector_t stop = {0, 0};
	waypoint_t* route1 = NULL;
	waypoint_t* route1s = NULL;
	waypoint_t* route2 = NULL;
	waypoint_t* route2s = NULL;
	char needJsonOutput = 0;

	struct timeval t1, t2;
	double elapsedTime;

	cluster_t* clusters = malloc (sizeof(cluster_t) * n * n);

	set(clusters, n, safety_radius);

	while (!quit) {
		safety_radius = GLOBALS.HEIGHT / (n + 1) / 5 ;
		SDL_FillRect( SDL_GetVideoSurface(), NULL, 0 );
		if (view_grid) {
			draw_grid(clusters, n);
		}

		if (view_points) {
			draw_clusters(clusters, n);
		}

		if (view_waypoints) {
			draw_route(route1, 0, 0, 255, 1);
			draw_route(route2, 255, 0, 0, 1);
		}

		if (view_smoothed) {
			draw_route(route1s, 0, 255, 255, 0);
			draw_route(route2s, 255, 255, 0, 0);
		}

		if (start.x != 0 && start.y != 0) {
			aacircleRGBA(screen, start.x, start.y, 2, 0, 255, 0, 255);
		}

		if (stop.x != 0 && stop.y != 0) {
			aacircleRGBA(screen, stop.x, stop.y, 2, 255, 0, 0, 255);
		}

		SDL_Flip(screen);
		SDL_WaitEvent(&e);
		needJsonOutput = view_json;
		switch (e.type) {
			case SDL_QUIT :
				quit = 1;
				break;
			case SDL_MOUSEBUTTONDOWN :
				switch (e.button.button) {
					case SDL_BUTTON_LEFT:
						start.x = e.button.x;
						start.y = e.button.y;
						break;
					case SDL_BUTTON_RIGHT:
						stop.x = e.button.x;
						stop.y = e.button.y;
						break;
				}
				if (start.x != 0 && start.y != 0 && stop.x != 0 && stop.y != 0) {
					fprintf(stderr, "\n\nRoute from (%f,%f) to (%f,%f)\n", start.x, start.y, stop.x, stop.y);

					gettimeofday(&t1, NULL);
					route1 = plotCourse(&start, &stop, clusters, n);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					fprintf(stderr, "plotCourse scanline based took %f ms\n", elapsedTime);

					gettimeofday(&t1, NULL);
					route1s = smooth(route1, 5);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					fprintf(stderr, "smoth scanline based route took %f ms\n", elapsedTime);

					gettimeofday(&t1, NULL);
					route2 = plotCourse_gridbased(&start, &stop, clusters, n);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					fprintf(stderr, "plotCourse gridline based took %f ms\n", elapsedTime);

					gettimeofday(&t1, NULL);
					route2s = smooth(route2, 5);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					fprintf(stderr, "smooth gridline based route took %f ms\n", elapsedTime);

					if(needJsonOutput) {
						json_update(route1s, route2s, n*n);
					}
				}
				break;
			case SDL_KEYDOWN :
				switch (e.key.keysym.sym) {
					case SDLK_p:
						view_points = !view_points;
						break;
					case SDLK_g:
					case SDLK_l:
						view_grid = !view_grid;
						break;
					case SDLK_w:
						view_waypoints = !view_waypoints;
						break;
					case SDLK_s:
						view_smoothed = !view_smoothed;
						break;
					case SDLK_j:
						view_json = !view_json;
						needJsonOutput = view_json;
						break;
					case SDLK_r:
						randomize(clusters, n, fac);
						break;
					case SDLK_BACKSPACE:
						set(clusters, n, safety_radius);
						break;
					case SDLK_q:
						quit = 1;
						break;
					case SDLK_UP:
						n++;
						clusters = realloc(clusters, sizeof(cluster_t) * n * n);
						fac = GLOBALS.WIDTH / (n + 1) * 0.3;
						set(clusters, n, safety_radius);
						break;
					case SDLK_DOWN:
						n--;
						clusters = realloc (clusters, sizeof(cluster_t) * n * n);
						fac = GLOBALS.WIDTH / (n + 1) * 0.3;
						set(clusters, n, safety_radius);
						break;
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
						fac *= 1.1;
						fprintf(stderr, "fac: %f\n", fac);
						set(clusters, n, safety_radius);
						randomize(clusters, n, fac);
						break;
					case SDLK_MINUS:
					case SDLK_KP_MINUS:
						fac /= 1.1;
						fprintf(stderr, "fac: %f\n", fac);
						set(clusters, n, safety_radius);
						randomize(clusters, n, fac);
						break;
					default:
						needJsonOutput = 0;
						break;
				}
				break;
			case SDL_VIDEORESIZE:
				GLOBALS.WIDTH  = e.resize.w;
				GLOBALS.HEIGHT = e.resize.h;
				sdl_screen_init();
				break;
			default :
				needJsonOutput = 0;
				break;
		}
		if(needJsonOutput) {
			json_output(clusters, n*n);
		}
	}

	free(clusters);

	return 0;
}
