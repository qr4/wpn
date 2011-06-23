#include "sdl.h"

SDL_Surface* screen;

SDL_Surface* asteroid_image;
SDL_Surface* explosion_image;
SDL_Surface* planet_image;
SDL_Surface* ship_small_image;
SDL_Surface* ship_medium_image;
SDL_Surface* shot_image;
SDL_Surface* slot_empty_image;
SDL_Surface* slot_L_image;
SDL_Surface* slot_R_image;
SDL_Surface* slot_T_image;

extern asteroid_t* asteroids;
extern explosion_t* explosions;
extern planet_t* planets;
extern ship_t* ships;
extern shot_t* shots;

extern int n_asteroids;
extern int n_explosions;
extern int n_planets;
extern int n_ships;
extern int n_shots;

unsigned int display_x = 640;
unsigned int display_y = 480;
extern bbox_t boundingbox;

float zoom;
float offset_x;
float offset_y;

float mag = 64;

void screen_init() {
	screen = SDL_SetVideoMode(display_x, display_y, 0, SDL_RESIZABLE | SDL_HWSURFACE | SDL_DOUBLEBUF);

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

void drawAsteroid(asteroid_t* a) {
	static float last_zoom = 1;
	static SDL_Surface* asteroid_sprite = NULL;

	if(!asteroid_sprite || (zoom != last_zoom)) {
		SDL_FreeSurface(asteroid_sprite);
		asteroid_sprite = zoomSurface(asteroid_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		last_zoom = zoom;
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

void drawShip(ship_t * s) {
	static float last_zoom = 1;
	static SDL_Surface* ship_small_sprite = NULL;
	static SDL_Surface* ship_medium_sprite = NULL;

	if(!ship_small_sprite || !ship_medium_sprite || (zoom != last_zoom)) {
		SDL_FreeSurface(ship_small_sprite);
		ship_small_sprite = zoomSurface(ship_small_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(ship_medium_sprite);
		ship_medium_sprite = zoomSurface(ship_medium_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		last_zoom = zoom;
	}

	if(s->active > 0) {
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
		}
	}
}

void drawSlot(float x, float y, char type) {
	static float last_zoom = 1;
	static SDL_Surface* slot_empty_sprite = NULL;
	static SDL_Surface* slot_L_sprite = NULL;
	static SDL_Surface* slot_R_sprite = NULL;
	static SDL_Surface* slot_T_sprite = NULL;

	if(!slot_empty_sprite || !slot_L_sprite || !slot_R_sprite || !slot_T_sprite || (zoom != last_zoom)) {
		SDL_FreeSurface(slot_empty_sprite);
		slot_empty_sprite = zoomSurface(slot_empty_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_L_sprite);
		slot_L_sprite = zoomSurface(slot_L_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_R_sprite);
		slot_R_sprite = zoomSurface(slot_R_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_T_sprite);
		slot_T_sprite = zoomSurface(slot_T_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		last_zoom = zoom;
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
	static SDL_Surface* planet_sprite = NULL;

	if(!planet_sprite || (zoom != last_zoom)) {
		SDL_FreeSurface(planet_sprite);
		planet_sprite = zoomSurface(planet_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		last_zoom = zoom;
	}

	if(p->active > 0) {
		SDL_Rect dst_rect;

		dst_rect.x = offset_x + (p->x - 8 * mag) * zoom;
		dst_rect.y = offset_y + (p->y - 8 * mag) * zoom;
		dst_rect.w = 16 * mag * zoom;
		dst_rect.h = 16 * mag * zoom;

		SDL_BlitSurface(planet_sprite, NULL, screen, &dst_rect);
	}
}

void drawExplosion(explosion_t* e) {
	static float last_zoom = 1;
	static SDL_Surface* explosion_sprite = NULL;
	int i;
	//unsigned int x,y,u;
	//char a;

	if(!explosion_sprite || (zoom != last_zoom)) {
		SDL_FreeSurface(explosion_sprite);
		explosion_sprite = zoomSurface(explosion_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		last_zoom = zoom;
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
	static SDL_Surface* shot_sprite = NULL;

	if(!shot_sprite || (zoom != last_zoom)) {
		SDL_FreeSurface(shot_sprite);
		shot_sprite = zoomSurface(shot_image, mag * zoom / 1.0, mag * zoom / 1.0, 0);
		last_zoom = zoom;
	}

	aalineRGBA(screen, offset_x + s->src_x * zoom, offset_y + s->src_y * zoom, offset_x + s->trg_x * zoom, offset_y + s->trg_y * zoom, 0, 255, 0, 128);

	SDL_Rect dst_rect;

	dst_rect.x = offset_x + (s->trg_x - 4 * mag) * zoom;
	dst_rect.y = offset_y + (s->trg_y - 4 * mag) * zoom;
	dst_rect.w = 8 * mag * zoom;
	dst_rect.h = 8 * mag * zoom;

	SDL_BlitSurface(shot_sprite, NULL, screen, &dst_rect);
}
