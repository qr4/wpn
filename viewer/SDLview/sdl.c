#include "sdl.h"
#include <SDL_ttf.h>

SDL_Surface* screen;

SDL_Surface* asteroid_image;
SDL_Surface* base_small_image;
SDL_Surface* base_medium_image;
SDL_Surface* base_large_image;
SDL_Surface* base_huge_image;
SDL_Surface* explosion_image;
SDL_Surface* planet_image;
SDL_Surface* ship_small_image;
SDL_Surface* ship_medium_image;
SDL_Surface* ship_large_image;
SDL_Surface* ship_huge_image;
SDL_Surface* shot_image;
SDL_Surface* slot_empty_image;
SDL_Surface* slot_L_image;
SDL_Surface* slot_R_image;
SDL_Surface* slot_T_image;

extern asteroid_t* asteroids;
extern base_t* bases;
extern explosion_t* explosions;
extern planet_t* planets;
extern player_t* players;
extern ship_t* ships;
extern shot_t* shots;

extern int n_asteroids;
extern int n_bases;
extern int n_explosions;
extern int n_planets;
extern int n_players;
extern int n_ships;
extern int n_shots;

unsigned int display_x = 640;
unsigned int display_y = 480;
extern bbox_t boundingbox;

float zoom;
float offset_x;
float offset_y;

extern int local_player;

#define default_mag 64
float mag = default_mag;


#define FONT_FILE "terminus.ttf"
#define FONT_SIZE 9
TTF_Font* font;
char show_text_name = 1;
char show_text_id = 0;
char show_text_coords = 0;

void screen_init() {
	screen = SDL_SetVideoMode(display_x, display_y, 0, SDL_RESIZABLE | SDL_SWSURFACE | SDL_DOUBLEBUF);

	if (screen == NULL) {
		fprintf(stderr, "SDL_SetVideoMode() failed: %s\n", SDL_GetError());
		exit(1);
	}
}

void SDLinit() {
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
		exit(1);
	}

	atexit(SDL_Quit);

	screen_init();

	/* Initialize font rendering */
	TTF_Init();
	atexit(TTF_Quit);
	font = TTF_OpenFont(FONT_FILE, FONT_SIZE);

	// load support for the JPG and PNG image formats
	int flags = IMG_INIT_JPG;
	int initted=IMG_Init(flags);
	if((initted & flags) != flags) {
		fprintf(stderr, "IMG_Init: Failed to load png support: %s\n", IMG_GetError());
		exit(1);
	}

	asteroid_image = IMG_LoadPNG_RW(SDL_RWFromFile("asteroid.png", "rb"));
	if(!asteroid_image) {
		printf("IMG_LoadPNG_RW: Loading asteroid image failed: %s\n", IMG_GetError());
		exit(1);
	}

	base_small_image = IMG_LoadPNG_RW(SDL_RWFromFile("base_small.png", "rb"));
	if(!base_small_image) {
		printf("IMG_LoadPNG_RW: Loading image for small base failed: %s\n", IMG_GetError());
		exit(1);
	}

	base_medium_image = IMG_LoadPNG_RW(SDL_RWFromFile("base_medium.png", "rb"));
	if(!base_medium_image) {
		printf("IMG_LoadPNG_RW: Loading image for medium base failed: %s\n", IMG_GetError());
		exit(1);
	}

	base_large_image = IMG_LoadPNG_RW(SDL_RWFromFile("base_large.png", "rb"));
	if(!base_large_image) {
		printf("IMG_LoadPNG_RW: Loading image for large base failed: %s\n", IMG_GetError());
		exit(1);
	}

	base_huge_image = IMG_LoadPNG_RW(SDL_RWFromFile("base_huge.png", "rb"));
	if(!base_huge_image) {
		printf("IMG_LoadPNG_RW: Loading image for huge base failed: %s\n", IMG_GetError());
		exit(1);
	}

	explosion_image = IMG_LoadPNG_RW(SDL_RWFromFile("explosion.png", "rb"));
	if(!explosion_image) {
		printf("IMG_LoadPNG_RW: Loading explosion image failed: %s\n", IMG_GetError());
		exit(1);
	}

	planet_image = IMG_LoadPNG_RW(SDL_RWFromFile("planet.png", "rb"));
	if(!planet_image) {
		printf("IMG_LoadPNG_RW: Loading planet image failed: %s\n", IMG_GetError());
		exit(1);
	}

	ship_small_image = IMG_LoadPNG_RW(SDL_RWFromFile("ship_small.png", "rb"));
	if(!ship_small_image) {
		printf("IMG_LoadPNG_RW: Loading image for small ship failed: %s\n", IMG_GetError());
		exit(1);
	}

	ship_medium_image = IMG_LoadPNG_RW(SDL_RWFromFile("ship_medium.png", "rb"));
	if(!ship_medium_image) {
		printf("IMG_LoadPNG_RW: Loading image for medium ship failed: %s\n", IMG_GetError());
		exit(1);
	}

	ship_large_image = IMG_LoadPNG_RW(SDL_RWFromFile("ship_large.png", "rb"));
	if(!ship_large_image) {
		printf("IMG_LoadPNG_RW: Loading image for large ship failed: %s\n", IMG_GetError());
		exit(1);
	}

	ship_huge_image = IMG_LoadPNG_RW(SDL_RWFromFile("ship_huge.png", "rb"));
	if(!ship_huge_image) {
		printf("IMG_LoadPNG_RW: Loading image for huge ship failed: %s\n", IMG_GetError());
		exit(1);
	}

	shot_image = IMG_LoadPNG_RW(SDL_RWFromFile("shot.png", "rb"));
	if(!shot_image) {
		printf("IMG_LoadPNG_RW: Loading shot image failed: %s\n", IMG_GetError());
		exit(1);
	}

	slot_empty_image = IMG_LoadPNG_RW(SDL_RWFromFile("slot_empty.png", "rb"));
	if(!slot_empty_image) {
		printf("IMG_LoadPNG_RW: Loading image of empty slot failed: %s\n", IMG_GetError());
		exit(1);
	}

	slot_L_image = IMG_LoadPNG_RW(SDL_RWFromFile("slot_L.png", "rb"));
	if(!slot_L_image) {
		printf("IMG_LoadPNG_RW: Loading image of laser slot failed: %s\n", IMG_GetError());
		exit(1);
	}

	slot_R_image = IMG_LoadPNG_RW(SDL_RWFromFile("slot_R.png", "rb"));
	if(!slot_R_image) {
		printf("IMG_LoadPNG_RW: Loading image of resource slot failed: %s\n", IMG_GetError());
		exit(1);
	}

	slot_T_image = IMG_LoadPNG_RW(SDL_RWFromFile("slot_T.png", "rb"));
	if(!slot_T_image) {
		printf("IMG_LoadPNG_RW: Loading image of thruster slot failed: %s\n", IMG_GetError());
		exit(1);
	}

	zoom = 1;
	offset_x = 0;
	offset_y = 0;
}

void checkSDLevent() {
	SDL_Event event;
	static Uint32 lastclickat = 0;

	if(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
						zoom *= 1.2;
						offset_x = display_x / 2 + (offset_x - display_x / 2) * 1.2;
						offset_y = display_y / 2 + (offset_y - display_y / 2) * 1.2;
						break;
					case SDLK_MINUS:
					case SDLK_KP_MINUS:
						zoom /= 1.2;
						offset_x = display_x / 2 + (offset_x - display_x / 2) / 1.2;
						offset_y = display_y / 2 + (offset_y - display_y / 2) / 1.2;
						break;
					case SDLK_m:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// M
							mag *= 1.5;
						} else {
							mag /= 1.5;
						}
						break;
					case SDLK_c:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// C
							show_text_coords = 0;
						} else {
							show_text_coords = 1;
						}
						break;
					case SDLK_i:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// I
							show_text_id = 0;
						} else {
							show_text_id = 1;
							show_text_name = 0;
						}
						break;
					case SDLK_n:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// N
							show_text_name = 0;
						} else {
							show_text_name = 1;
							show_text_id = 0;
						}
						break;
					case SDLK_q:
						fprintf(stderr, "Closing by user request\n");
						exit(0);
					default:
						break;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(event.button.button == 1) { // left button
					if(SDL_GetTicks() - lastclickat < 200) {
						// center
						offset_x = offset_x - event.button.x + display_x / 2;
						offset_y = offset_y - event.button.y + display_y / 2;
						// and zoom
						zoom *= 1.2;
						offset_x = display_x / 2 + (offset_x - display_x/2) * 1.2;
						offset_y = display_y / 2 + (offset_y - display_y/2) * 1.2;
						// disarm
						lastclickat = 0;
					} else {
						// rearm double click detection
						lastclickat = SDL_GetTicks();
					}
				} else {
					// disarm doubleclick detection
					lastclickat = 0;
				}
				break;
			case SDL_MOUSEMOTION:
				if(event.motion.state & 1) {
					offset_x += event.motion.xrel;
					offset_y += event.motion.yrel;
				}
				break;
            case SDL_VIDEORESIZE:
				boundingbox.xmax = display_x = event.resize.w;
				boundingbox.ymax = display_y = event.resize.h;
				screen_init();
				break;
			case SDL_QUIT:
				fprintf(stderr, "Closing by user request\n");
				exit(0);
		}
	}
}

void SDLplot() {
	int i;

	SDL_FillRect(SDL_GetVideoSurface(), NULL, 0);

	//fprintf(stderr, "Plotting %d asteroids\n", n_asteroids);
	for(i = 0; i < n_asteroids; i++) {
		drawAsteroid(&(asteroids[i]));
	}

	//fprintf(stderr, "Plotting %d planets\n", n_planets);
	for(i = 0; i < n_planets; i++) {
		drawPlanet(&(planets[i]));
	}

	for(i = 0; i < n_bases; i++) {
		drawBase(&(bases[i]));
	}

	for(i = 0; i < n_ships; i++) {
		drawShip(&(ships[i]));
	}

	for(i = 0; i < n_shots; i++) {
		drawShot(&(shots[i]));
	}

	for(i = 0; i < n_explosions; i++) {
		drawExplosion(&(explosions[i]));
	}

	SDL_Flip(screen);
}

Uint8 red_from_H(double h) {
	if(h < 0) {
		return 0;
	}
	while(h > 360) {
		h -= 360;
	}
	if(h<60) {
		return 255;
	} else if(h<120) {
		return (120-h)*4.25;
	} else if(h<240) {
		return 0;
	} else if(h<300) {
		return (h-240)*4.25;
	} else {
		return 255;
	}
}

Uint8 green_from_H(double h) {
	if(h < 0) {
		return 0;
	}
	while(h > 360) {
		h -= 360;
	}
	if(h<60) {
		return h*4.25;
	} else if(h<180) {
		return 255;
	} else if(h<240) {
		return (240-h)*4.25;
	} else {
		return 0;
	}
}

Uint8 blue_from_H(double h) {
	if(h < 0) {
		return 0;
	}
	while(h > 360) {
		h -= 360;
	}
	if(h<120) {
		return 0;
	} else if(h<180) {
		return (h-120)*4.25;
	} else if(h<300) {
		return 255;
	} else {
		return (300-h)*4.25;
	}
}

double player_to_h(int playerid) {
	if(local_player != -1) {
		if(playerid == local_player) {
			return 120; // green
		} else {
			return 0; // red
		}
	} else {
		// yay coder colors
		return 293*(playerid-100);
	}
}

void drawText(int x,int y, char* text) {
	SDL_Color text_color = {255,255,255,0};
	SDL_Color bg = {0,0,0,0};
	SDL_Surface* text_surf;
	SDL_Rect pos;

	if(x < 0 || y < 0 || x > screen->w || y > screen->h) {
		return;
	}

	text_surf = TTF_RenderText_Shaded(font, text, text_color, bg);

	pos.x = x;
	pos.y = y;
	pos.w = text_surf->w;
	pos.h = text_surf->h;

	if(x < text_surf->w || y < text_surf->h || x > screen->w - text_surf->w || y > screen->h - text_surf->h) {
		return;
	}

	SDL_BlitSurface(text_surf, NULL, screen, &pos);

	SDL_FreeSurface(text_surf);
}

void drawAsteroid(asteroid_t* a) {
	static float last_zoom = 1;
	static float last_mag = default_mag;
	static SDL_Surface* asteroid_sprite = NULL;

	if(!asteroid_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(asteroid_sprite);
		asteroid_sprite = zoomSurface(asteroid_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		last_zoom = zoom;
		last_mag = mag;
	}

	if(a->active > 0) {
		SDL_Rect dst_rect;

		dst_rect.x = offset_x + (a->x - 4 * mag) * zoom;
		dst_rect.y = offset_y + (a->y - 4 * mag) * zoom;
		dst_rect.w = 8 * mag * zoom;
		dst_rect.h = 8 * mag * zoom;

		SDL_BlitSurface(asteroid_sprite, NULL, screen, &dst_rect);
		drawSlot(offset_x + (a->x - 1.9 * mag) * zoom, offset_y + (a->y - 1.1 * mag) * zoom, a->contents[0]);
		drawSlot(offset_x + (a->x - 0.7 * mag) * zoom, offset_y + (a->y - 1.1 * mag) * zoom, a->contents[1]);
		drawSlot(offset_x + (a->x + 0.6 * mag) * zoom, offset_y + (a->y - 1.1 * mag) * zoom, a->contents[2]);
		drawSlot(offset_x + (a->x - 1.9 * mag) * zoom, offset_y + (a->y + 0.2 * mag) * zoom, a->contents[3]);
		drawSlot(offset_x + (a->x - 0.7 * mag) * zoom, offset_y + (a->y + 0.2 * mag) * zoom, a->contents[4]);
		drawSlot(offset_x + (a->x + 0.6 * mag) * zoom, offset_y + (a->y + 0.2 * mag) * zoom, a->contents[5]);
		drawSlot(offset_x + (a->x + 2.05 * mag) * zoom, offset_y + (a->y + 0.95 * mag) * zoom, a->contents[6]);
	}
}

void drawHalo(double x, double y, double r, double h) {
	filledCircleRGBA(screen, x, y, r,       red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 0.5, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 1.0, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 1.4, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 2.0, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 2.8, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 4.0, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 5.7, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
	filledCircleRGBA(screen, x, y, r - 8.0, red_from_H(h), green_from_H(h), blue_from_H(h), 6);
}

void drawBase(base_t * b) {
	static float last_zoom = 1;
	static float last_mag = default_mag;
	static SDL_Surface* base_small_sprite = NULL;
	static SDL_Surface* base_medium_sprite = NULL;
	static SDL_Surface* base_large_sprite = NULL;
	static SDL_Surface* base_huge_sprite = NULL;

	if(!base_small_sprite || !base_medium_sprite || !base_large_sprite || !base_huge_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(base_small_sprite);
		base_small_sprite = zoomSurface(base_small_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(base_medium_sprite);
		base_medium_sprite = zoomSurface(base_medium_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(base_large_sprite);
		base_large_sprite = zoomSurface(base_large_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(base_huge_sprite);
		base_huge_sprite = zoomSurface(base_huge_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		last_zoom = zoom;
		last_mag = mag;
	}

	if(b->active > 0) {
		SDL_Rect dst_rect;

		dst_rect.x = offset_x + (b->x - 6 * mag) * zoom;
		dst_rect.y = offset_y + (b->y - 6 * mag) * zoom;
		dst_rect.w = 12 * mag * zoom;
		dst_rect.h = 12 * mag * zoom;

		if(b->size > 0 && b-> size <= 3) {
			SDL_BlitSurface(base_small_sprite, NULL, screen, &dst_rect);
			drawSlot(offset_x + (b->x - .45 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[0]);
			drawSlot(offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y + 0.8 * mag) * zoom, b->contents[1]);
			drawSlot(offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y + 0.8 * mag) * zoom, b->contents[2]);
		} else if (b->size <= 6) {
			SDL_BlitSurface(base_medium_sprite, NULL, screen, &dst_rect);
			drawSlot(offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y - 1.5 * mag) * zoom, b->contents[0]);
			drawSlot(offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y - 1.5 * mag) * zoom, b->contents[1]);
			drawSlot(offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y - 0.4 * mag) * zoom, b->contents[2]);
			drawSlot(offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y - 0.4 * mag) * zoom, b->contents[3]);
			drawSlot(offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[4]);
			drawSlot(offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[5]);
		} else if (b->size <= 12) {
			SDL_BlitSurface(base_large_sprite, NULL, screen, &dst_rect);
			drawSlot(offset_x + (b->x - 4.5 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[0]);
			drawSlot(offset_x + (b->x - 3.4 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[1]);
			drawSlot(offset_x + (b->x - 4.5 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[2]);
			drawSlot(offset_x + (b->x - 3.4 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[3]);
			drawSlot(offset_x + (b->x - 1.1 * mag) * zoom, offset_y + (b->y - 1.8 * mag) * zoom, b->contents[4]);
			drawSlot(offset_x + (b->x + 0.0 * mag) * zoom, offset_y + (b->y - 1.8 * mag) * zoom, b->contents[5]);
			drawSlot(offset_x + (b->x - 1.1 * mag) * zoom, offset_y + (b->y - 0.7 * mag) * zoom, b->contents[6]);
			drawSlot(offset_x + (b->x + 0.0 * mag) * zoom, offset_y + (b->y - 0.7 * mag) * zoom, b->contents[7]);
			drawSlot(offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[8]);
			drawSlot(offset_x + (b->x + 3.6 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[9]);
			drawSlot(offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[10]);
			drawSlot(offset_x + (b->x + 3.6 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[11]);
		} else if (b->size <= 24) {
			SDL_BlitSurface(base_huge_sprite, NULL, screen, &dst_rect);
			drawSlot(offset_x + (b->x - 4.8 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[0]);
			drawSlot(offset_x + (b->x - 3.5 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[1]);
			drawSlot(offset_x + (b->x - 5.4 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[2]);
			drawSlot(offset_x + (b->x - 2.9 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[3]);
			drawSlot(offset_x + (b->x - 4.8 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[4]);
			drawSlot(offset_x + (b->x - 3.5 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[5]);
			drawSlot(offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[6]);
			drawSlot(offset_x + (b->x + 3.8 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[7]);
			drawSlot(offset_x + (b->x + 1.9 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[8]);
			drawSlot(offset_x + (b->x + 4.4 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[9]);
			drawSlot(offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[10]);
			drawSlot(offset_x + (b->x + 3.8 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[11]);
			drawSlot(offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y - 5.3 * mag) * zoom, b->contents[12]);
			drawSlot(offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y - 5.3 * mag) * zoom, b->contents[13]);
			drawSlot(offset_x + (b->x - 1.7 * mag) * zoom, offset_y + (b->y - 4.1 * mag) * zoom, b->contents[14]);
			drawSlot(offset_x + (b->x + 0.6 * mag) * zoom, offset_y + (b->y - 4.1 * mag) * zoom, b->contents[15]);
			drawSlot(offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y - 2.9 * mag) * zoom, b->contents[16]);
			drawSlot(offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y - 2.9 * mag) * zoom, b->contents[17]);
			drawSlot(offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y + 1.9 * mag) * zoom, b->contents[18]);
			drawSlot(offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y + 1.9 * mag) * zoom, b->contents[19]);
			drawSlot(offset_x + (b->x - 1.7 * mag) * zoom, offset_y + (b->y + 3.1 * mag) * zoom, b->contents[20]);
			drawSlot(offset_x + (b->x + 0.6 * mag) * zoom, offset_y + (b->y + 3.1 * mag) * zoom, b->contents[21]);
			drawSlot(offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y + 4.3 * mag) * zoom, b->contents[22]);
			drawSlot(offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y + 4.3 * mag) * zoom, b->contents[23]);
		}
	}
}

void drawShip(ship_t * s) {
	static float last_zoom = 1;
	static float last_mag = default_mag;
	static SDL_Surface* ship_small_sprite = NULL;
	static SDL_Surface* ship_medium_sprite = NULL;
	static SDL_Surface* ship_large_sprite = NULL;
	static SDL_Surface* ship_huge_sprite = NULL;

	if(!ship_small_sprite || !ship_medium_sprite || !ship_large_sprite || !ship_huge_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(ship_small_sprite);
		ship_small_sprite = zoomSurface(ship_small_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(ship_medium_sprite);
		ship_medium_sprite = zoomSurface(ship_medium_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(ship_large_sprite);
		ship_large_sprite = zoomSurface(ship_large_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(ship_huge_sprite);
		ship_huge_sprite = zoomSurface(ship_huge_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		last_zoom = zoom;
		last_mag = mag;
	}

	if(s->active > 0) {
		if(s->owner != 0) {
			drawHalo(offset_x + s->x * zoom, offset_y + s->y * zoom, 10 * mag * zoom, player_to_h(s->owner));
		}

		SDL_Rect dst_rect;

		dst_rect.x = offset_x + (s->x - 3 * mag) * zoom;
		dst_rect.y = offset_y + (s->y - 3 * mag) * zoom;
		dst_rect.w = 6 * mag * zoom;
		dst_rect.h = 6 * mag * zoom;

		if(s->size > 0 && s-> size <= 3) {
			SDL_BlitSurface(ship_small_sprite, NULL, screen, &dst_rect);
			drawSlot(offset_x + (s->x - 0.48 * mag) * zoom, offset_y + (s->y + 1.2 * mag) * zoom, s->contents[0]);
			drawSlot(offset_x + (s->x - 0.48 * mag) * zoom, offset_y + (s->y + 0.0 * mag) * zoom, s->contents[1]);
			drawSlot(offset_x + (s->x - 0.48 * mag) * zoom, offset_y + (s->y - 1.3 * mag) * zoom, s->contents[2]);
		} else if (s->size <= 6) {
			SDL_BlitSurface(ship_medium_sprite, NULL, screen, &dst_rect);
			drawSlot(offset_x + (s->x - 1.1 * mag) * zoom, offset_y + (s->y + 1.27 * mag) * zoom, s->contents[0]);
			drawSlot(offset_x + (s->x + 0.13 * mag) * zoom, offset_y + (s->y + 1.27 * mag) * zoom, s->contents[1]);
			drawSlot(offset_x + (s->x - 1.1 * mag) * zoom, offset_y + (s->y + 0.10 * mag) * zoom, s->contents[2]);
			drawSlot(offset_x + (s->x + 0.13 * mag) * zoom, offset_y + (s->y + 0.10 * mag) * zoom, s->contents[3]);
			drawSlot(offset_x + (s->x - 1.1 * mag) * zoom, offset_y + (s->y - 1.165 * mag) * zoom, s->contents[4]);
			drawSlot(offset_x + (s->x + 0.13 * mag) * zoom, offset_y + (s->y - 1.165 * mag) * zoom, s->contents[5]);
		} else if (s->size <= 12) {
			SDL_BlitSurface(ship_large_sprite, NULL, screen, &dst_rect);
		} else if (s->size <= 24) {
			dst_rect.x -= 3 * mag * zoom;
			dst_rect.y -= 3 * mag * zoom;
			dst_rect.w += 6 * mag * zoom;
			dst_rect.h += 6 * mag * zoom;
			SDL_BlitSurface(ship_huge_sprite, NULL, screen, &dst_rect);
		}
	}
}

void drawSlot(float x, float y, char type) {
	static float last_zoom = 1;
	static float last_mag = default_mag;
	static SDL_Surface* slot_empty_sprite = NULL;
	static SDL_Surface* slot_L_sprite = NULL;
	static SDL_Surface* slot_R_sprite = NULL;
	static SDL_Surface* slot_T_sprite = NULL;

	if(!slot_empty_sprite || !slot_L_sprite || !slot_R_sprite || !slot_T_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(slot_empty_sprite);
		slot_empty_sprite = zoomSurface(slot_empty_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_L_sprite);
		slot_L_sprite = zoomSurface(slot_L_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_R_sprite);
		slot_R_sprite = zoomSurface(slot_R_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_T_sprite);
		slot_T_sprite = zoomSurface(slot_T_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		last_zoom = zoom;
		last_mag = mag;
	}

	SDL_Rect dst_rect;

	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = zoom;
	dst_rect.h = zoom;

	if(type == 'L') {
		SDL_BlitSurface(slot_L_sprite, NULL, screen, &dst_rect);
	} else if (type == 'R') {
		SDL_BlitSurface(slot_R_sprite, NULL, screen, &dst_rect);
	} else if (type == 'T') {
		SDL_BlitSurface(slot_T_sprite, NULL, screen, &dst_rect);
	} else if (type == ' ' || type == 0) {
		SDL_BlitSurface(slot_empty_sprite, NULL, screen, &dst_rect);
	}
}

void drawPlanet(planet_t* p) {
	static float last_zoom = 1;
	static float last_mag = default_mag;
	static SDL_Surface* planet_sprite = NULL;
	char* text = NULL;

	if(!planet_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(planet_sprite);
		planet_sprite = zoomSurface(planet_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		last_zoom = zoom;
		last_mag = mag;
	}

	if(p->active > 0) {
		if(p->owner != 0) {
			drawHalo(offset_x + p->x * zoom, offset_y + p->y * zoom, 25 * mag * zoom, player_to_h(p->owner));
		}

		SDL_Rect dst_rect;

		dst_rect.x = offset_x + (p->x - 8 * mag) * zoom;
		dst_rect.y = offset_y + (p->y - 8 * mag) * zoom;
		dst_rect.w = 16 * mag * zoom;
		dst_rect.h = 16 * mag * zoom;

		SDL_BlitSurface(planet_sprite, NULL, screen, &dst_rect);

		if(show_text_name) {
			if(p->owner > 0) {
				for(int i = 0; i < n_players; i++) {
					if(players[i].id == p->owner) {
						asprintf(&text, "%s", players[i].name);
						break;
					}
				}
				if(!text) {
					asprintf(&text, "(%i)", p->owner);
				}
				drawText(dst_rect.x+14, dst_rect.y-14, text);
				free(text);
			}
		} else if(show_text_id) {
			if(p->owner > 0) {
				asprintf(&text, "%i", p->owner);
				drawText(dst_rect.x+14, dst_rect.y-14, text);
				free(text);
			}
		}
		if(show_text_coords) {
			asprintf(&text, "% 6.0f", p->x);
			drawText(dst_rect.x+14, dst_rect.y, text);
			free(text);
			asprintf(&text, "% 6.0f", p->y);
			drawText(dst_rect.x+14, dst_rect.y+10, text);
			free(text);
		}
	}
}

void drawExplosion(explosion_t* e) {
	static float last_zoom = 1;
	static float last_mag = default_mag;
	static SDL_Surface* explosion_sprite = NULL;
	int i;
	//unsigned int x,y,u;
	//char a;

	if(!explosion_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(explosion_sprite);
		explosion_sprite = zoomSurface(explosion_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		last_zoom = zoom;
		last_mag = mag;
	}

	if(e->strength > 0) {
		SDL_Rect dst_rect;

		dst_rect.x = offset_x + (e->x - 4 * mag) * zoom;
		dst_rect.y = offset_y + (e->y - 4 * mag) * zoom;
		dst_rect.w = 8 * mag * zoom;
		dst_rect.h = 8 * mag * zoom;

		// Klappt so ned, weil gleichzeitig per-Surface und per-Pixel Alpha-Wert ned unterstützt wird
		// SDL_SetAlpha(explosion_sprite, SDL_RLEACCEL | SDL_SRCALPHA, e->strength);
		// SDL_BlitSurface(explosion_sprite, NULL, screen, &dst_rect);

		// Danke liebes SDL daß man auch Colorkeying und alphablending ned kombinieren kann

		// Schreckliches Pixelpushing, das vermutlich dauernd zerbricht. Japp tut es. Bis zum Segfault
		/*
		SDL_PixelFormat *fmt = explosion_sprite->format;
		SDL_Surface* tmp = SDL_CreateRGBSurface(SDL_SRCALPHA, 8*zoom, 8*zoom, fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
		for(x = 0; x < 8*zoom; x++) {
			for(y = 0; y < 8*zoom; y++) {
				u = ((Uint32*)explosion_sprite->pixels)[y*(explosion_sprite->pitch/sizeof(Uint32)) + x];
				a = (u & 0xff000000)>>24;
				a = a * e->strength / 255;
				u = (u & 0x00ffffff) | ((a<<24)& 0xff000000);
				((Uint32*)tmp->pixels)[y*(tmp->pitch/sizeof(Uint32)) + x] = u;
			}
		}
		SDL_BlitSurface(tmp, NULL, screen, &dst_rect);
		*/

		// Terrible, terrbible cheating
		for(i = 0; i < e->strength * e->strength / 1024; i++) {
			SDL_BlitSurface(explosion_sprite, NULL, screen, &dst_rect);
		}
		e->strength -= 1;
	}

}

void drawShot(shot_t* s) {
	static float last_zoom = 1;
	static float last_mag = default_mag;
	static SDL_Surface* shot_sprite = NULL;

	if(!shot_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(shot_sprite);
		shot_sprite = zoomSurface(shot_image, mag * zoom / 1.0, mag * zoom / 1.0, 0);
		last_zoom = zoom;
		last_mag = mag;
	}

	aalineRGBA(screen, offset_x + s->src_x * zoom, offset_y + s->src_y * zoom, offset_x + s->trg_x * zoom, offset_y + s->trg_y * zoom, 0, 255, 0, 128);

	SDL_Rect dst_rect;

	dst_rect.x = offset_x + (s->trg_x - 4 * mag) * zoom;
	dst_rect.y = offset_y + (s->trg_y - 4 * mag) * zoom;
	dst_rect.w = 8 * mag * zoom;
	dst_rect.h = 8 * mag * zoom;

	SDL_BlitSurface(shot_sprite, NULL, screen, &dst_rect);
}
