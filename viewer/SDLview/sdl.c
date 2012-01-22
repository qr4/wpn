#include <string.h>
#include "sdl.h"
#include <SDL_ttf.h>
#include <SDL/SDL_gfxBlitFunc.h>
#include "layerrenderers.h"

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

extern state_t state;
extern options_t options;
extern options_t options_old;
void drawSlot(float x, float y, char type);
static layer_t layers[] = {
	{draw_influence,  NULL, true},
	{draw_asteroids,  NULL, true},
	{draw_planets,    NULL, true},
	{draw_bases,      NULL, true},
	{draw_ships,      NULL, true},
	{draw_shots,      NULL, true},
	{draw_explosions, NULL, true},
};
static size_t n_layers = sizeof(layers) / sizeof(layer_t);

json_int_t follow_ship = 0;

#define FONT_FILE "terminus.ttf"
#define FONT_SIZE 9
#define asprintf(...) if(asprintf(__VA_ARGS__))
TTF_Font* font;

void screen_init() {
	screen = SDL_SetVideoMode(options.display_x, options.display_y, 32, SDL_RESIZABLE | SDL_SWSURFACE | SDL_DOUBLEBUF);

	if (screen == NULL) {
		fprintf(stderr, "SDL_SetVideoMode() failed: %s\n", SDL_GetError());
		exit(1);
	}
}

SDL_Surface *load_img(const char *filename) {
	SDL_RWops *file  = SDL_RWFromFile(filename, "rb");
	SDL_Surface *t   = IMG_LoadTyped_RW(file, 1, "PNG");
	SDL_Surface *ret;

	if (t == NULL) {
		fprintf(stderr, "Failed to load %s: %s\n", filename, IMG_GetError());
		exit(1);
	}

	ret = SDL_DisplayFormatAlpha(t);
	SDL_FreeSurface(t);
	SDL_SetAlpha(ret, SDL_SRCALPHA | SDL_RLEACCEL, SDL_ALPHA_OPAQUE);

	return ret;
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

	asteroid_image    = load_img("asteroid.png");
	base_small_image  = load_img("base_small.png");
	base_medium_image = load_img("base_medium.png");
	base_large_image  = load_img("base_large.png");
	base_huge_image   = load_img("base_huge.png");
	explosion_image   = load_img("explosion.png");
	planet_image      = load_img("planet.png");
	ship_small_image  = load_img("ship_small.png");
	ship_medium_image = load_img("ship_medium.png");
	ship_large_image  = load_img("ship_large.png");
	ship_huge_image   = load_img("ship_huge.png");
	shot_image        = load_img("shot.png");
	slot_empty_image  = load_img("slot_empty.png");
	slot_L_image      = load_img("slot_L.png");
	slot_R_image      = load_img("slot_R.png");
	slot_T_image      = load_img("slot_T.png");
}

void checkSDLevent() {
	SDL_Event event;
	static Uint32 lastclickat = 0;


	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							options.influence_threshhold /= 1.2;
						} else {
							options.zoom *= 1.2;
							options.offset_x = options.display_x / 2 + (options.offset_x - options.display_x / 2) * 1.2;
							options.offset_y = options.display_y / 2 + (options.offset_y - options.display_y / 2) * 1.2;
						}
						break;
					case SDLK_MINUS:
					case SDLK_KP_MINUS:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							options.influence_threshhold *= 1.2;
						} else {
							options.zoom /= 1.2;
							options.offset_x = options.display_x / 2 + (options.offset_x - options.display_x / 2) / 1.2;
							options.offset_y = options.display_y / 2 + (options.offset_y - options.display_y / 2) / 1.2;
						}
						break;
					case SDLK_m:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// M
							options.mag *= 1.5;
						} else {
							options.mag /= 1.5;
						}
						break;
					case SDLK_c:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// C
							options.show_text_coords = 0;
						} else {
							options.show_text_coords = 1;
						}
						break;
					case SDLK_i:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// I
							options.show_text_id = 0;
						} else {
							options.show_text_id = 1;
							options.show_text_name = 0;
						}
						break;
					case SDLK_n:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// N
							options.show_text_name = 0;
						} else {
							options.show_text_name = 1;
							options.show_text_id = 0;
						}
						break;
					case SDLK_p:
						if((event.key.keysym.mod & KMOD_LSHIFT) || (event.key.keysym.mod & KMOD_RSHIFT)) {
							// P
							options.show_influence = 0;
						} else {
							options.show_influence = 1;
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
				if(event.button.button == SDL_BUTTON_LEFT) {
					if(SDL_GetTicks() - lastclickat < 200) {
						// center
						options.offset_x = options.offset_x - event.button.x + options.display_x / 2;
						options.offset_y = options.offset_y - event.button.y + options.display_y / 2;
						// and zoom
						options.zoom *= 1.2;
						options.offset_x = options.display_x / 2 + (options.offset_x - options.display_x/2) * 1.2;
						options.offset_y = options.display_y / 2 + (options.offset_y - options.display_y/2) * 1.2;
						// disarm
						lastclickat = 0;
					} else {
						// rearm double click detection
						lastclickat = SDL_GetTicks();
					}
				} else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
					options.zoom /= 1.2;
					options.offset_x = options.display_x / 2 + (options.offset_x - options.display_x / 2) / 1.2;
					options.offset_y = options.display_y / 2 + (options.offset_y - options.display_y / 2) / 1.2;
				} else if (event.button.button == SDL_BUTTON_WHEELUP) {
					options.zoom *= 1.2;
					options.offset_x = options.display_x / 2 + (options.offset_x - options.display_x / 2) * 1.2;
					options.offset_y = options.display_y / 2 + (options.offset_y - options.display_y / 2) * 1.2;
				} else {
					// disarm doubleclick detection
					lastclickat = 0;
					if(event.button.button == SDL_BUTTON_RIGHT) {
						find_object_at(event.button.x, event.button.y);
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				layers[0].active = true;
				break;
			case SDL_MOUSEMOTION:
				options.mouse_pos_x = event.motion.x;
				options.mouse_pos_y = event.motion.y;
				if(event.motion.state & 1) {
					layers[0].active = false;
					options.offset_x += event.motion.xrel;
					options.offset_y += event.motion.yrel;
				}
				break;
            case SDL_VIDEORESIZE:
				state.boundingbox.xmax = options.display_x = event.resize.w;
				state.boundingbox.ymax = options.display_y = event.resize.h;
				screen_init();
				break;
			case SDL_QUIT:
				fprintf(stderr, "Closing by user request\n");
				exit(0);
		}
	}
}

static SDL_Surface *create_buffer(uint32_t bg) {
	SDL_Surface *scr;
	SDL_Surface *ret;
	scr = SDL_GetVideoSurface();
	ret = SDL_DisplayFormatAlpha(scr);
	SDL_SetAlpha(ret, SDL_SRCALPHA | SDL_RLEACCEL, SDL_ALPHA_TRANSPARENT);
	SDL_FillRect(ret, NULL, bg);
	return ret;
}

void SDLplot() {
	size_t i;
	SDL_Surface *scr = SDL_GetVideoSurface();
	uint32_t bgcolor = SDL_MapRGBA(scr->format, 0, 0, 10, 255);
	SDL_FillRect(scr, NULL, bgcolor);

	zoom_textures();

#pragma omp parallel for
	for (i = 0; i < n_layers; i++) {
		if (layers[i].active == true) {
			layers[i].buffer = create_buffer(bgcolor);
			layers[i].f(layers[i].buffer);
		}
	}

	for (i = 0; i < n_layers; i++) {
		if (layers[i].active == true) {
			SDL_BlitSurface(layers[i].buffer, NULL, scr, NULL);
			SDL_FreeSurface(layers[i].buffer);
		}
	}

	char* pos;
	asprintf(&pos, "% 6.0f % 6.0f", (options.mouse_pos_x-options.offset_x)/options.zoom, (options.mouse_pos_y-options.offset_y)/options.zoom);
	drawText(scr, 16, 10, pos);
	free(pos);

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
	if(options.local_player != -1) {
		if(playerid == options.local_player) {
			return 120; // green
		} else {
			return 0; // red
		}
	} else {
		// yay coder colors
		return 293*(playerid-100);
	}
}

void drawText(SDL_Surface *buffer, int x,int y, char* text) {
	SDL_Color text_color = {255,255,255,0};
	SDL_Color bg = {0,0,0,0};
	SDL_Surface* text_surf;
	SDL_Rect pos;

	if(x < 0 || y < 0 || x > buffer->w || y > buffer->h) {
		return;
	}

	/* Render the ttf string */
	text_surf = TTF_RenderText_Shaded(font, text, text_color, bg);

	pos.x = x;
	pos.y = y;
	pos.w = text_surf->w;
	pos.h = text_surf->h;

	/* Blit only if the target position is completely within the screen bounds */
	if(x < buffer->w - text_surf->w && y < buffer->h - text_surf->h) {
		SDL_BlitSurface(text_surf, NULL, buffer, &pos);
	}

	/* Let us not leak memory today */
	SDL_FreeSurface(text_surf);
}

static inline double dist(double x1, double y1, double x2, double y2) {
	return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}



void find_object_at(int click_x, int click_y) {
	const float zoom = options.zoom;
	const float offset_x = options.offset_x;
	const float offset_y = options.offset_y;
	double radius = 50;
	follow_ship = 0;
	for(size_t i = 0; i < state.ships.n; i++) {
		if(state.ships.ships[i].active == 0) {
			continue;
		}
		double d = dist(click_x, click_y, offset_x + state.ships.ships[i].x * zoom, offset_y + state.ships.ships[i].y * zoom);
		if(d < radius) {
			radius = d;
			follow_ship = state.ships.ships[i].id;
		}
	}
}

