#include <stdio.h>
#include <sys/time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <errno.h>

#include "globals.h"
#include "screen.h"
#include "route.h"

/* TODO:
 *
 * a global scene for easy rerender
 * fast image switch
 */

SDL_Surface *screen;
int safety_radius;

void sdl_setup() {
	const SDL_VideoInfo *info;
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		ERROR("SDL_Init() failed: %s\n", SDL_GetError());
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
		ERROR("SDL_SetVideoMode() failed: %s\n", SDL_GetError());
		exit(1);
	}
}

void draw_line(pixel_t *p1, pixel_t *p2) {
	aalineRGBA(screen, p1->x, p1->y, p2->x, p2->y, 255, 255, 255, 255);
}

void draw_blob(pixel_t *p1) {
	aacircleRGBA(screen, p1->x, p1->y, 2, 255, 255, 255, 255);
	aacircleRGBA(screen, p1->x, p1->y, safety_radius, 64, 64, 64, 255);
}

void draw_points(pixel_t *points, int n) {
	int x, y;

	for (y = 0; y < n; y++) {
		for (x = 0; x < n; x++) {
			draw_blob(&points[y*n + x]);
		}
	}
}

void draw_grid(pixel_t *points, int n) {
	int x, y;

	for (y = 0; y < n - 1; y++) {
		for (x = 0; x < n - 1; x++) {
			draw_line(&points[y*n + x], &points[y*n + x + 1]);
			draw_line(&points[y*n + x], &points[(y + 1)*n + x]);
		}
		draw_line(&points[y*n + x], &points[(y + 1)*n + x]);
	}
	for (x = 0; x < n - 1; x++) {
		draw_line(&points[y*n + x], &points[y*n + x + 1]);
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

void randomize(pixel_t *points, int n, float fac) {
	int i;

	for (i = 0; i < n * n; i++) {
		points[i].x += (((float) rand() / RAND_MAX) - 0.5) * 2 * fac;
		points[i].y += (((float) rand() / RAND_MAX) - 0.5) * 2 * fac;
	}
	
}

void set(pixel_t *points, int n) {
	int x, y;

	for (y = 0; y < n; y++) {
		for (x = 0; x < n; x++) {
			points[y*n + x].x = GLOBALS.WIDTH  * (x + 1) / (n + 1);
			points[y*n + x].y = GLOBALS.HEIGHT * (y + 1) / (n + 1);
		}
	}
}

int main_loop() {
	int quit = 0;
	int n = 16;
	int view_grid = 0;
	int view_points  = 1;
	int view_waypoints = 1;
	int view_smoothed = 1;
	float fac = GLOBALS.WIDTH / (n + 1) * 0.3;
	SDL_Event e;
	pixel_t *points;
	pixel_t start = {0, 0};
	pixel_t stop = {0, 0};
	waypoint_t* route1 = NULL;
	waypoint_t* route1s = NULL;
	waypoint_t* route2 = NULL;
	waypoint_t* route2s = NULL;

	struct timeval t1, t2;
	double elapsedTime;

	points = (pixel_t *) malloc (sizeof(pixel_t) * n * n);

	set(points, n);

	while (!quit) {
		safety_radius = GLOBALS.HEIGHT / (n + 1) / 5 ;
		SDL_FillRect( SDL_GetVideoSurface(), NULL, 0 );
		if (view_grid) {
			draw_grid(points, n);
		}

		if (view_points) {
			draw_points(points, n);
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
					printf("\n\nRoute from (%f,%f) to (%f,%f)\n", start.x, start.y, stop.x, stop.y);

					gettimeofday(&t1, NULL);
					route1 = plotCourse(&start, &stop, points, n);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					printf("plotCourse scanline based took %f ms\n", elapsedTime);

					gettimeofday(&t1, NULL);
					route1s = smooth(route1, 5);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					printf("smoth scanline based route took %f ms\n", elapsedTime);

					gettimeofday(&t1, NULL);
					route2 = plotCourse_gridbased(&start, &stop, points, n);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					printf("plotCourse gridline based took %f ms\n", elapsedTime);

					gettimeofday(&t1, NULL);
					route2s = smooth(route2, 5);
					gettimeofday(&t2, NULL);
					elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
					elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
					printf("smooth gridline based route took %f ms\n", elapsedTime);
					/*
					waypoint_t* r = route;
					while (r != NULL) {
						printf("Waypoint (%f,%f)\n", r->point.x, r->point.y);
						r = r->next;
					}
					*/
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
					case SDLK_r:
						randomize(points, n, fac);
						break;
					case SDLK_BACKSPACE:
						set(points, n);
						break;
					case SDLK_q:
						quit = 1;
						break;
					case SDLK_UP:
						n++;
						points = (pixel_t *) realloc (points, sizeof(pixel_t) * n * n);
						fac = GLOBALS.WIDTH / (n + 1) * 0.3;
						set(points, n);
						break;
					case SDLK_DOWN:
						n--;
						points = (pixel_t *) realloc (points, sizeof(pixel_t) * n * n);
						fac = GLOBALS.WIDTH / (n + 1) * 0.3;
						set(points, n);
						break;
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
						fac *= 1.1;
						printf("fac: %f\n", fac);
						set(points, n);
						randomize(points, n, fac);
						break;
					case SDLK_MINUS:
					case SDLK_KP_MINUS:
						fac /= 1.1;
						printf("fac: %f\n", fac);
						set(points, n);
						randomize(points, n, fac);
						break;
					default:
						break;
				}
				break;
			case SDL_VIDEORESIZE:
				GLOBALS.WIDTH  = e.resize.w;
				GLOBALS.HEIGHT = e.resize.h;
				sdl_screen_init();
				break;
			default :
				break;
		}
	}

	free(points);

	return 0;
}
