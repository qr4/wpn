#include <math.h>
#include <SDL/SDL_gfxBlitFunc.h>
#include <pthread.h>

#include "layerrenderers.h"
#include "snapshot.h"

extern state_t state;
#define asprintf(...) if(asprintf(__VA_ARGS__))

extern options_t options;
extern options_t options_old;

static inline double dist(double x1, double y1, double x2, double y2) {
	return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

static SDL_Surface* slot_empty_sprite = NULL;
static SDL_Surface* slot_L_sprite = NULL;
static SDL_Surface* slot_R_sprite = NULL;
static SDL_Surface* slot_T_sprite = NULL;

static SDL_Surface* base_small_sprite = NULL;
static SDL_Surface* base_medium_sprite = NULL;
static SDL_Surface* base_large_sprite = NULL;
static SDL_Surface* base_huge_sprite = NULL;

static SDL_Surface* planet_sprite = NULL;
static SDL_Surface* asteroid_sprite = NULL;

static SDL_Surface* ship_small_sprite = NULL;
static SDL_Surface* ship_medium_sprite = NULL;
static SDL_Surface* ship_large_sprite = NULL;
static SDL_Surface* ship_huge_sprite = NULL;

static SDL_Surface* shot_sprite = NULL;
static SDL_Surface* explosion_sprite = NULL;

static SDL_Surface* influence_cache = NULL;
static int position_of_max(double values[], int n) {
	int pos_max = -1;
	double max = -HUGE_VAL;

	for (int i = 0; i < n; i++) {
		if (values[i] > max) {
			max = values[i];
			pos_max = i;
		}
	}

	return pos_max;
}

static bool same_storage(const storage_t *s1, const storage_t *s2, size_t size) {
	if (s1 == NULL || s2 == NULL) return false;
	if (s1->n != s2->n) return false;
	if (size != 0) {
		if (memcmp(s1->data, s2->data, size * s1->n) != 0) return false;
	}
	return true;
}

static uint32_t color_from_owner(const SDL_Surface *buffer, int owner) {
	double h = player_to_h(owner);
	return SDL_MapRGBA(buffer->format, red_from_H(h), green_from_H(h), blue_from_H(h), 32);
}

static void *render_influence(void *arg) {
	SDL_Surface *buffer = (SDL_Surface *) arg;
	double left, up;
	double pos_x, pos_y;
	double d;
	size_t x, y;
	int max_owner;
	int min_owner;
	int pos_max;
	size_t id_range;
	snapshot_t *snapshot = snapshot_getcurrent();
	const base_t *bases = snapshot->state.bases.bases;
	const size_t n_bases = snapshot->state.bases.n;
	min_owner = max_owner = bases[0].owner;

	for (size_t i = 1; i < n_bases; i++) {
		if (bases[i].owner < min_owner) {
			min_owner = bases[i].owner;
		} else if (bases[i].owner > max_owner) {
			max_owner = bases[i].owner;
		}
	}

	id_range = max_owner - min_owner + 1;

	uint32_t colormap[id_range];
	for (size_t i = 0; i < id_range; i++) {
		colormap[i] = color_from_owner(arg, i + min_owner);
	}

	d    = 1 / snapshot->options.zoom;
	left = -snapshot->options.offset_x * d;
	up   = -snapshot->options.offset_y * d;

	for (y = 0, pos_y = up; y < buffer->h; y+=1, pos_y = up + y * d) {
		for (x = 0, pos_x = left; x < buffer->w; x+=1, pos_x = left + x * d) {
			double influence[id_range];
			memset(influence, 0, sizeof(double) * (id_range));

			for (size_t i = 0; i < n_bases ; i++) {
				influence[bases[i].owner - min_owner] += bases[i].size / dist(pos_x, pos_y, bases[i].x, bases[i].y);
			}

			pos_max = position_of_max(influence, id_range);

			if (influence[pos_max] >= snapshot->options.influence_threshhold) {
				((uint32_t *) buffer->pixels)[buffer->w * y + x] = colormap[pos_max];
			}
		}
	}
	snapshot_release(snapshot);

	return buffer;
}

typedef enum {
	UNINITIALIZED,
	RUNNING,
	COMPLETE,
} thread_stat_t;


void draw_influence(SDL_Surface *buffer) {
	static snapshot_t *snapshot = NULL;
	static snapshot_t *snapshot_old = NULL;
	static pthread_t influence_render_thread;
	static thread_stat_t thread_status = UNINITIALIZED;
	SDL_Surface *thread_ret;

	snapshot = snapshot_getcurrent();

	if (snapshot->state.bases.n < 1 || snapshot->options.show_influence == false) {
		goto cleanup;
	}

	if (thread_status == RUNNING && pthread_tryjoin_np(influence_render_thread, (void **) &thread_ret) == 0) {
		SDL_FreeSurface(influence_cache);
		influence_cache = thread_ret;
		thread_status = COMPLETE;
	}

	if (
			   snapshot_old                           != NULL
			&& influence_cache                        != NULL
			&& same_storage(&snapshot->state.bases, &snapshot_old->state.bases, sizeof(base_t))
			&& snapshot->options.show_influence       == snapshot_old->options.show_influence
			&& snapshot->options.offset_x             == snapshot_old->options.offset_x
			&& snapshot->options.offset_y             == snapshot_old->options.offset_y
			&& snapshot->options.influence_threshhold == snapshot_old->options.influence_threshhold
			&& buffer->w                              == influence_cache->w
			&& buffer->h                              == influence_cache->h
		) {
		goto blit_cache;
	}

	if (thread_status != RUNNING) {
		SDL_Surface *t;
		snapshot_release(snapshot_old);
		snapshot_old = snapshot_getcurrent();

		t = SDL_ConvertSurface(buffer, buffer->format, buffer->flags);

		pthread_create(&influence_render_thread, NULL, render_influence, t);
		thread_status = RUNNING;
	}

blit_cache:
	SDL_gfxBlitRGBA(influence_cache, NULL, buffer, NULL);
cleanup:
	snapshot_release(snapshot);
}

static void draw_slot(SDL_Surface *buffer, float x, float y, char type) {
	SDL_Surface *slot;

	SDL_Rect dst_rect;

	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = 0;
	dst_rect.h = 0;

	switch (type) {
		case 'L':
			slot = slot_L_sprite;
			break;
		case 'R':
			slot = slot_R_sprite;
			break;
		case 'T':
			slot = slot_T_sprite;
			break;
		case ' ':
			slot = slot_empty_sprite;
			break;
		default:
			return;
	}

	if (slot->w < 2 || slot->h < 2) return;
	SDL_gfxBlitRGBA(slot, NULL, buffer, &dst_rect);
}

// TODO:
// prerender Halos
static void drawHalo(SDL_Surface *buffer, double x, double y, double r, double h) {
	return;
	for (size_t i = 0; i < 10; i++) {
		double f = pow(0.9, ((double) i / 10));
		filledCircleRGBA(buffer, x, y, r * f, red_from_H(h), green_from_H(h), blue_from_H(h), i * 10);
	}
}


void draw_asteroids(SDL_Surface *buffer) {
	const float mag = options.mag;
	const float zoom = options.zoom;

	for (size_t i = 0; i < state.asteroids.n; i++) {
		const asteroid_t *a = &(state.asteroids.asteroids[i]);
		if(a->active > 0) {
			SDL_Rect dst_rect;

			dst_rect.x = options.offset_x + (a->x - 4 * mag) * zoom;
			dst_rect.y = options.offset_y + (a->y - 4 * mag) * zoom;
			dst_rect.w = 8 * mag * zoom;
			dst_rect.h = 8 * mag * zoom;

			SDL_gfxBlitRGBA(asteroid_sprite, NULL, buffer, &dst_rect);
			draw_slot(buffer, options.offset_x + (a->x - 1.9 * mag) * zoom,  options.offset_y + (a->y - 1.1 * mag) * zoom, a->contents[0]);
			draw_slot(buffer, options.offset_x + (a->x - 0.7 * mag) * zoom,  options.offset_y + (a->y - 1.1 * mag) * zoom, a->contents[1]);
			draw_slot(buffer, options.offset_x + (a->x + 0.6 * mag) * zoom,  options.offset_y + (a->y - 1.1 * mag) * zoom, a->contents[2]);
			draw_slot(buffer, options.offset_x + (a->x - 1.9 * mag) * zoom,  options.offset_y + (a->y + 0.2 * mag) * zoom, a->contents[3]);
			draw_slot(buffer, options.offset_x + (a->x - 0.7 * mag) * zoom,  options.offset_y + (a->y + 0.2 * mag) * zoom, a->contents[4]);
			draw_slot(buffer, options.offset_x + (a->x + 0.6 * mag) * zoom,  options.offset_y + (a->y + 0.2 * mag) * zoom, a->contents[5]);
			draw_slot(buffer, options.offset_x + (a->x + 2.05 * mag) * zoom, options.offset_y + (a->y + 0.95 * mag) * zoom, a->contents[6]);
		}
	}
}


void draw_planets(SDL_Surface *buffer) {
	char* text = NULL;

	const float mag = options.mag;
	const float zoom = options.zoom;
	const float offset_x = options.offset_x;
	const float offset_y = options.offset_y;
	const size_t n_players = state.players.n;
	const player_t *players = state.players.players;

	for (size_t i = 0; i < state.planets.n; i++) {
		const planet_t *p = &(state.planets.planets[i]);
		if(p->active > 0) {
			if(p->owner != 0) {
				drawHalo(buffer, offset_x + p->x * zoom, offset_y + p->y * zoom, 25 * mag * zoom, player_to_h(p->owner));
			}

			SDL_Rect dst_rect;

			dst_rect.x = offset_x + (p->x - 8 * mag) * zoom;
			dst_rect.y = offset_y + (p->y - 8 * mag) * zoom;
			dst_rect.w = 16 * mag * zoom;
			dst_rect.h = 16 * mag * zoom;

			SDL_gfxBlitRGBA(planet_sprite, NULL, buffer, &dst_rect);

			/* Since BlitSurface modifies the rectangle to show the actual blitting area, re-set the coordinates */
			dst_rect.x = offset_x + (p->x - 8 * mag) * zoom;
			dst_rect.y = offset_y + (p->y - 8 * mag) * zoom;

			if(options.show_text_name) {
				if(p->owner > 0) {
					for(size_t j = 0; j < n_players; j++) {
						if(players[j].id == p->owner) {
							if(strlen(players[j].name) > 10) {
								asprintf(&text, "%.6s...", players[j].name);
							} else {
								asprintf(&text, "%s", players[j].name);
							}
							break;
						}
					}
					if(!text) {
						asprintf(&text, "(%i)", p->owner);
					}
					drawText(buffer, dst_rect.x+14, dst_rect.y-14, text);
					free(text);
				}
			} else if(options.show_text_id) {
				if(p->owner > 0) {
					asprintf(&text, "%i", p->owner);
					drawText(buffer, dst_rect.x+14, dst_rect.y-14, text);
					free(text);
				}
			}
			if(options.show_text_coords) {
				asprintf(&text, "% 6.0f", p->x);
				drawText(buffer, dst_rect.x+14, dst_rect.y, text);
				free(text);
				asprintf(&text, "% 6.0f", p->y);
				drawText(buffer, dst_rect.x+14, dst_rect.y+10, text);
				free(text);
			}
		}
	}
}

void draw_bases(SDL_Surface *buffer) {

	const float mag = options.mag;
	const float zoom = options.zoom;
	const float offset_x = options.offset_x;
	const float offset_y = options.offset_y;

	for(size_t i = 0; i < state.bases.n; i++) {
		base_t *b = &(state.bases.bases[i]);
		if(b->active > 0) {
			SDL_Rect dst_rect;

			dst_rect.x = offset_x + (b->x - 6 * mag) * zoom;
			dst_rect.y = offset_y + (b->y - 6 * mag) * zoom;
			dst_rect.w = 12 * mag * zoom;
			dst_rect.h = 12 * mag * zoom;

			if(b->size > 0 && b-> size <= 3) {
				SDL_gfxBlitRGBA(base_small_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (b->x - .45 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[0]);
				draw_slot(buffer, offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y + 0.8 * mag) * zoom, b->contents[1]);
				draw_slot(buffer, offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y + 0.8 * mag) * zoom, b->contents[2]);
			} else if (b->size <= 6) {
				SDL_gfxBlitRGBA(base_medium_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y - 1.5 * mag) * zoom, b->contents[0]);
				draw_slot(buffer, offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y - 1.5 * mag) * zoom, b->contents[1]);
				draw_slot(buffer, offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y - 0.4 * mag) * zoom, b->contents[2]);
				draw_slot(buffer, offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y - 0.4 * mag) * zoom, b->contents[3]);
				draw_slot(buffer, offset_x + (b->x - 1.0 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[4]);
				draw_slot(buffer, offset_x + (b->x + 0.1 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[5]);
			} else if (b->size <= 12) {
				SDL_gfxBlitRGBA(base_large_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (b->x - 4.5 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[0]);
				draw_slot(buffer, offset_x + (b->x - 3.4 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[1]);
				draw_slot(buffer, offset_x + (b->x - 4.5 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[2]);
				draw_slot(buffer, offset_x + (b->x - 3.4 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[3]);
				draw_slot(buffer, offset_x + (b->x - 1.1 * mag) * zoom, offset_y + (b->y - 1.8 * mag) * zoom, b->contents[4]);
				draw_slot(buffer, offset_x + (b->x + 0.0 * mag) * zoom, offset_y + (b->y - 1.8 * mag) * zoom, b->contents[5]);
				draw_slot(buffer, offset_x + (b->x - 1.1 * mag) * zoom, offset_y + (b->y - 0.7 * mag) * zoom, b->contents[6]);
				draw_slot(buffer, offset_x + (b->x + 0.0 * mag) * zoom, offset_y + (b->y - 0.7 * mag) * zoom, b->contents[7]);
				draw_slot(buffer, offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[8]);
				draw_slot(buffer, offset_x + (b->x + 3.6 * mag) * zoom, offset_y + (b->y - 1.3 * mag) * zoom, b->contents[9]);
				draw_slot(buffer, offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[10]);
				draw_slot(buffer, offset_x + (b->x + 3.6 * mag) * zoom, offset_y + (b->y - 0.2 * mag) * zoom, b->contents[11]);
			} else if (b->size <= 24) {
				SDL_gfxBlitRGBA(base_huge_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (b->x - 4.8 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[0]);
				draw_slot(buffer, offset_x + (b->x - 3.5 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[1]);
				draw_slot(buffer, offset_x + (b->x - 5.4 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[2]);
				draw_slot(buffer, offset_x + (b->x - 2.9 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[3]);
				draw_slot(buffer, offset_x + (b->x - 4.8 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[4]);
				draw_slot(buffer, offset_x + (b->x - 3.5 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[5]);
				draw_slot(buffer, offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[6]);
				draw_slot(buffer, offset_x + (b->x + 3.8 * mag) * zoom, offset_y + (b->y - 1.7 * mag) * zoom, b->contents[7]);
				draw_slot(buffer, offset_x + (b->x + 1.9 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[8]);
				draw_slot(buffer, offset_x + (b->x + 4.4 * mag) * zoom, offset_y + (b->y - 0.5 * mag) * zoom, b->contents[9]);
				draw_slot(buffer, offset_x + (b->x + 2.5 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[10]);
				draw_slot(buffer, offset_x + (b->x + 3.8 * mag) * zoom, offset_y + (b->y + 0.7 * mag) * zoom, b->contents[11]);
				draw_slot(buffer, offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y - 5.3 * mag) * zoom, b->contents[12]);
				draw_slot(buffer, offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y - 5.3 * mag) * zoom, b->contents[13]);
				draw_slot(buffer, offset_x + (b->x - 1.7 * mag) * zoom, offset_y + (b->y - 4.1 * mag) * zoom, b->contents[14]);
				draw_slot(buffer, offset_x + (b->x + 0.6 * mag) * zoom, offset_y + (b->y - 4.1 * mag) * zoom, b->contents[15]);
				draw_slot(buffer, offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y - 2.9 * mag) * zoom, b->contents[16]);
				draw_slot(buffer, offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y - 2.9 * mag) * zoom, b->contents[17]);
				draw_slot(buffer, offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y + 1.9 * mag) * zoom, b->contents[18]);
				draw_slot(buffer, offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y + 1.9 * mag) * zoom, b->contents[19]);
				draw_slot(buffer, offset_x + (b->x - 1.7 * mag) * zoom, offset_y + (b->y + 3.1 * mag) * zoom, b->contents[20]);
				draw_slot(buffer, offset_x + (b->x + 0.6 * mag) * zoom, offset_y + (b->y + 3.1 * mag) * zoom, b->contents[21]);
				draw_slot(buffer, offset_x + (b->x - 1.2 * mag) * zoom, offset_y + (b->y + 4.3 * mag) * zoom, b->contents[22]);
				draw_slot(buffer, offset_x + (b->x + 0.2 * mag) * zoom, offset_y + (b->y + 4.3 * mag) * zoom, b->contents[23]);
			}
		}
	}
}

void draw_ships(SDL_Surface *buffer) {
	const float mag = options.mag;
	const float zoom = options.zoom;
	const float offset_x = options.offset_x;
	const float offset_y = options.offset_y;

	for(size_t i = 0; i < state.ships.n; i++) {
		const ship_t *s = &(state.ships.ships[i]);
		if(s->active > 0) {
			if(s->owner != 0) {
				drawHalo(buffer, offset_x + s->x * zoom, offset_y + s->y * zoom, 10 * mag * zoom, player_to_h(s->owner));
			}

			SDL_Rect dst_rect;

			dst_rect.x = offset_x + (s->x - 3 * mag) * zoom;
			dst_rect.y = offset_y + (s->y - 3 * mag) * zoom;
			dst_rect.w = 6 * mag * zoom;
			dst_rect.h = 6 * mag * zoom;

			if(s->size > 0 && s-> size <= 3) {
				SDL_gfxBlitRGBA(ship_small_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (s->x - 0.48 * mag) * zoom, offset_y + (s->y + 1.2 * mag) * zoom, s->contents[0]);
				draw_slot(buffer, offset_x + (s->x - 0.48 * mag) * zoom, offset_y + (s->y + 0.0 * mag) * zoom, s->contents[1]);
				draw_slot(buffer, offset_x + (s->x - 0.48 * mag) * zoom, offset_y + (s->y - 1.3 * mag) * zoom, s->contents[2]);
			} else if (s->size <= 6) {
				SDL_gfxBlitRGBA(ship_medium_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (s->x - 1.1 * mag) * zoom, offset_y + (s->y + 1.27 * mag) * zoom, s->contents[0]);
				draw_slot(buffer, offset_x + (s->x + 0.13 * mag) * zoom, offset_y + (s->y + 1.27 * mag) * zoom, s->contents[1]);
				draw_slot(buffer, offset_x + (s->x - 1.1 * mag) * zoom, offset_y + (s->y + 0.10 * mag) * zoom, s->contents[2]);
				draw_slot(buffer, offset_x + (s->x + 0.13 * mag) * zoom, offset_y + (s->y + 0.10 * mag) * zoom, s->contents[3]);
				draw_slot(buffer, offset_x + (s->x - 1.1 * mag) * zoom, offset_y + (s->y - 1.165 * mag) * zoom, s->contents[4]);
				draw_slot(buffer, offset_x + (s->x + 0.13 * mag) * zoom, offset_y + (s->y - 1.165 * mag) * zoom, s->contents[5]);
			} else if (s->size <= 12) {
				SDL_gfxBlitRGBA(ship_large_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (s->x - 2.1 * mag) * zoom, offset_y + (s->y + 3.4 * mag) * zoom, s->contents[0]);
				draw_slot(buffer, offset_x + (s->x - 2.1 * mag) * zoom, offset_y + (s->y + 0.0 * mag) * zoom, s->contents[10]);
				draw_slot(buffer, offset_x + (s->x - 0.6 * mag) * zoom, offset_y + (s->y - 1.6 * mag) * zoom, s->contents[2]);
				draw_slot(buffer, offset_x + (s->x - 0.6 * mag) * zoom, offset_y + (s->y - 0.5 * mag) * zoom, s->contents[4]);
				draw_slot(buffer, offset_x + (s->x - 0.6 * mag) * zoom, offset_y + (s->y + 0.6 * mag) * zoom, s->contents[6]);
				draw_slot(buffer, offset_x + (s->x - 0.6 * mag) * zoom, offset_y + (s->y + 1.7 * mag) * zoom, s->contents[8]);
				draw_slot(buffer, offset_x + (s->x + 1.6 * mag) * zoom, offset_y + (s->y - 1.6 * mag) * zoom, s->contents[3]);
				draw_slot(buffer, offset_x + (s->x + 1.6 * mag) * zoom, offset_y + (s->y - 0.5 * mag) * zoom, s->contents[5]);
				draw_slot(buffer, offset_x + (s->x + 1.6 * mag) * zoom, offset_y + (s->y + 0.6 * mag) * zoom, s->contents[7]);
				draw_slot(buffer, offset_x + (s->x + 1.6 * mag) * zoom, offset_y + (s->y + 1.7 * mag) * zoom, s->contents[9]);
				draw_slot(buffer, offset_x + (s->x + 3.1 * mag) * zoom, offset_y + (s->y + 0.0 * mag) * zoom, s->contents[11]);
				draw_slot(buffer, offset_x + (s->x + 3.1 * mag) * zoom, offset_y + (s->y + 3.4 * mag) * zoom, s->contents[1]);
			} else if (s->size <= 24) {
				dst_rect.x -= 3 * mag * zoom;
				dst_rect.y -= 3 * mag * zoom;
				dst_rect.w += 6 * mag * zoom;
				dst_rect.h += 6 * mag * zoom;
				SDL_gfxBlitRGBA(ship_huge_sprite, NULL, buffer, &dst_rect);
				draw_slot(buffer, offset_x + (s->x - 4 * mag) * zoom, offset_y + (s->y + 4 * mag) * zoom, s->contents[0]);
				draw_slot(buffer, offset_x + (s->x + 3 * mag) * zoom, offset_y + (s->y + 4 * mag) * zoom, s->contents[1]);

				draw_slot(buffer, offset_x + (s->x - 3.1 * mag) * zoom, offset_y + (s->y - 1.6 * mag) * zoom, s->contents[2]);
				draw_slot(buffer, offset_x + (s->x - 3.1 * mag) * zoom, offset_y + (s->y - 0.5 * mag) * zoom, s->contents[7]);
				draw_slot(buffer, offset_x + (s->x - 3.1 * mag) * zoom, offset_y + (s->y + 0.6 * mag) * zoom, s->contents[12]);
				draw_slot(buffer, offset_x + (s->x - 3.1 * mag) * zoom, offset_y + (s->y + 1.7 * mag) * zoom, s->contents[17]);

				draw_slot(buffer, offset_x + (s->x - 1.9 * mag) * zoom, offset_y + (s->y - 2.7 * mag) * zoom, s->contents[3]);
				draw_slot(buffer, offset_x + (s->x - 1.9 * mag) * zoom, offset_y + (s->y - 1.6 * mag) * zoom, s->contents[8]);
				draw_slot(buffer, offset_x + (s->x - 1.9 * mag) * zoom, offset_y + (s->y - 0.5 * mag) * zoom, s->contents[13]);
				draw_slot(buffer, offset_x + (s->x - 1.9 * mag) * zoom, offset_y + (s->y + 0.6 * mag) * zoom, s->contents[18]);
				draw_slot(buffer, offset_x + (s->x - 1.9 * mag) * zoom, offset_y + (s->y + 1.7 * mag) * zoom, s->contents[22]);

				draw_slot(buffer, offset_x + (s->x - 0.5 * mag) * zoom, offset_y + (s->y - 2.7 * mag) * zoom, s->contents[4]);
				draw_slot(buffer, offset_x + (s->x - 0.5 * mag) * zoom, offset_y + (s->y - 1.6 * mag) * zoom, s->contents[9]);
				draw_slot(buffer, offset_x + (s->x - 0.5 * mag) * zoom, offset_y + (s->y - 0.5 * mag) * zoom, s->contents[14]);
				draw_slot(buffer, offset_x + (s->x - 0.5 * mag) * zoom, offset_y + (s->y + 0.6 * mag) * zoom, s->contents[19]);

				draw_slot(buffer, offset_x + (s->x + 0.9 * mag) * zoom, offset_y + (s->y - 2.7 * mag) * zoom, s->contents[5]);
				draw_slot(buffer, offset_x + (s->x + 0.9 * mag) * zoom, offset_y + (s->y - 1.6 * mag) * zoom, s->contents[10]);
				draw_slot(buffer, offset_x + (s->x + 0.9 * mag) * zoom, offset_y + (s->y - 0.5 * mag) * zoom, s->contents[15]);
				draw_slot(buffer, offset_x + (s->x + 0.9 * mag) * zoom, offset_y + (s->y + 0.6 * mag) * zoom, s->contents[20]);
				draw_slot(buffer, offset_x + (s->x + 0.9 * mag) * zoom, offset_y + (s->y + 1.7 * mag) * zoom, s->contents[23]);

				draw_slot(buffer, offset_x + (s->x + 2.1 * mag) * zoom, offset_y + (s->y - 1.7 * mag) * zoom, s->contents[6]);
				draw_slot(buffer, offset_x + (s->x + 2.1 * mag) * zoom, offset_y + (s->y - 0.6 * mag) * zoom, s->contents[11]);
				draw_slot(buffer, offset_x + (s->x + 2.1 * mag) * zoom, offset_y + (s->y + 0.5 * mag) * zoom, s->contents[16]);
				draw_slot(buffer, offset_x + (s->x + 2.1 * mag) * zoom, offset_y + (s->y + 1.6 * mag) * zoom, s->contents[21]);
			}
		}
	}
}

void draw_shots(SDL_Surface *buffer) {
	const float mag = options.mag;
	const float zoom = options.zoom;
	const float offset_x = options.offset_x;
	const float offset_y = options.offset_y;

	for(size_t i = 0; i < state.shots.n; i++) {
		shot_t *s = &(state.shots.shots[i]);
		if(s->strength <= 0) {
			return;
		}
		s->strength--;

		//uint8_t width = 0.25*mag*zoom;
		//if(width < 1) {
		//	width = 1;
		//} else if(width > 16) {
		//	width = 16;
		//}
		// This hangs indefinitely once in a while....
		//thickLineRGBA(screen, offset_x + s->src_x * zoom, offset_y + s->src_y * zoom, offset_x + s->trg_x * zoom, offset_y + s->trg_y * zoom, width, 0, 255, 0, 128);
		aalineRGBA(buffer, offset_x + s->src_x * zoom, offset_y + s->src_y * zoom, offset_x + s->trg_x * zoom, offset_y + s->trg_y * zoom, 0, 255, 0, 128);

		SDL_Rect dst_rect;

		dst_rect.x = offset_x + (s->trg_x - 4 * mag) * zoom;
		dst_rect.y = offset_y + (s->trg_y - 4 * mag) * zoom;
		dst_rect.w = 4 * mag * zoom;
		dst_rect.h = 4 * mag * zoom;

		SDL_gfxBlitRGBA(shot_sprite, NULL, buffer, &dst_rect);
	}
}

void draw_explosions(SDL_Surface *buffer) {
	const float mag = options.mag;
	const float zoom = options.zoom;
	const float offset_x = options.offset_x;
	const float offset_y = options.offset_y;
	int i;
	//unsigned int x,y,u;
	//char a;

	for(i = 0; i < state.explosions.n; i++) {
		explosion_t *e = &(state.explosions.explosions[i]);

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
			for(i = 0; i < (e->strength * e->strength) / 4096; i++) {
				SDL_gfxBlitRGBA(explosion_sprite, NULL, buffer, &dst_rect);
			}
			e->strength -= 3;
		}
	}
}

static void print_surface_info(SDL_Surface *s) {
	fprintf(stderr, "w: %d, h: %d", s->w, s->h);
}

void zoom_textures() {
	bool changed = false;
	const float last_zoom = options_old.zoom;
	const float last_mag = options_old.mag;
	const float mag = options.mag;
	const float zoom = options.zoom;

	if(!base_small_sprite || !base_medium_sprite || !base_large_sprite || !base_huge_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(base_small_sprite);
		base_small_sprite = zoomSurface(base_small_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(base_medium_sprite);
		base_medium_sprite = zoomSurface(base_medium_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(base_large_sprite);
		base_large_sprite = zoomSurface(base_large_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(base_huge_sprite);
		base_huge_sprite = zoomSurface(base_huge_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);

		changed = true;
	}

	if(!planet_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(planet_sprite);
		planet_sprite = zoomSurface(planet_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		changed = true;
	}

	if(!asteroid_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(asteroid_sprite);
		asteroid_sprite = zoomSurface(asteroid_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		changed = true;
	}

	if(!slot_empty_sprite || !slot_L_sprite || !slot_R_sprite || !slot_T_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(slot_empty_sprite);
		slot_empty_sprite = zoomSurface(slot_empty_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_L_sprite);
		slot_L_sprite = zoomSurface(slot_L_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_R_sprite);
		slot_R_sprite = zoomSurface(slot_R_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		SDL_FreeSurface(slot_T_sprite);
		slot_T_sprite = zoomSurface(slot_T_image, mag * zoom / 16.0, mag * zoom / 16.0, 0);
		changed = true;
	}

	if(!ship_small_sprite || !ship_medium_sprite || !ship_large_sprite || !ship_huge_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(ship_small_sprite);
		ship_small_sprite = zoomSurface(ship_small_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(ship_medium_sprite);
		ship_medium_sprite = zoomSurface(ship_medium_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(ship_large_sprite);
		ship_large_sprite = zoomSurface(ship_large_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
		SDL_FreeSurface(ship_huge_sprite);
		ship_huge_sprite = zoomSurface(ship_huge_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
	}

	if(!shot_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(shot_sprite);
		shot_sprite = zoomSurface(shot_image, mag * zoom / 1.0, mag * zoom / 1.0, 0);
	}

	if(!explosion_sprite || (zoom != last_zoom) || (mag != last_mag)) {
		SDL_FreeSurface(explosion_sprite);
		explosion_sprite = zoomSurface(explosion_image, mag * zoom / 8.0, mag * zoom / 8.0, 0);
	}


	if (changed) {

		fprintf(stderr, "\n\n\n");
		fprintf(stderr, "last_zoom: %f\n", last_zoom);
		fprintf(stderr, "zoom: %f\n", zoom);
		fprintf(stderr, "last_mag: %f\n", last_mag);
		fprintf(stderr, "mag: %f\n", mag);
		fprintf(stderr, "\n\n");
#define pinfo(surface)\
		fprintf(stderr, "\n" #surface ": ");\
		print_surface_info(surface);


		fprintf(stderr, "zoom_factor: %f", mag * zoom / 16.0);
		pinfo(slot_empty_image);
		pinfo(slot_empty_sprite);
		pinfo(slot_L_sprite);
		pinfo(slot_R_sprite);
		pinfo(slot_T_sprite);
		pinfo(base_small_sprite);
		pinfo(base_medium_sprite);
		pinfo(base_large_sprite);
		pinfo(base_huge_sprite);
		pinfo(planet_sprite);
		pinfo(asteroid_sprite);
#undef pinfo
	}
}

