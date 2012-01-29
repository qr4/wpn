#include <math.h>

#include "layerrenderers.h"

extern state_t state;
extern options_t options;
extern options_t options_old;

static inline double dist(double x1, double y1, double x2, double y2) {
		return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

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

void draw_influence(SDL_Surface *buffer) {
	double left, up;
	double pos_x, pos_y;
	double d;
	size_t x, y;
	int max_owner;
	int min_owner;
	int pos_max;
	int owner;
	size_t id_range;

	const base_t *bases = state.bases.bases;
	const size_t n_bases = state.bases.n;

	if (state.bases.n < 1) {
		return;
	}

	min_owner = max_owner = bases[0].owner;

	for (size_t i = 1; i < n_bases; i++) {
		if (bases[i].owner < min_owner) {
			min_owner = bases[i].owner;
		} else if (bases[i].owner > max_owner) {
			max_owner = bases[i].owner;
		}
	}

	id_range = max_owner - min_owner + 1;

	d    = 1 / options.zoom;
	left = -options.offset_x * d;
	up   = -options.offset_y * d;

	for (y = 0, pos_y = up; y < buffer->h; y+=10, pos_y = up + y * d) {
		for (x = 0, pos_x = left; x < buffer->w; x+=10, pos_x = left + x * d) {
			double influence[id_range];
			memset(influence, 0, sizeof(double) * (id_range));

			for (size_t i = 0; i < n_bases ; i++) {
				influence[bases[i].owner - min_owner] += bases[i].size / dist(pos_x, pos_y, bases[i].x, bases[i].y);
			}

			pos_max = position_of_max(influence, id_range);
			owner   = min_owner + pos_max;


			if (influence[pos_max] >= options.influence_threshhold) {
				double h = player_to_h(owner);

				((uint32_t *) buffer->pixels)[buffer->w * y + x] = SDL_MapRGB(buffer->format, red_from_H(h), green_from_H(h), blue_from_H(h));
			}
		}
	}
}
