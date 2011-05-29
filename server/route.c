#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "physics.h"
#include "route.h"
#include "types.h"

extern map_t map;

waypoint_t* go_around(vector_t* A, vector_t* B, entity_t* C, double r) {
	vector_t X;
	X.v = A->v + (v2d) {r, r} * (B->v - A->v);
	double d = vector_dist(&X, &(C->pos));
	vector_t W;
	W.v = C->pos.v + (X.v - C->pos.v) * (v2d) {1.01, 1.01} * (v2d) {C->radius,  C->radius} / (v2d) {d, d};
	waypoint_t* wp = create_waypoint(W.x, W.y, 0, 0, 0, WP_TURN_VIA);
	wp->obs = C->pos;
	return wp;
}

waypoint_t* route(vector_t* start, vector_t* stop, entity_t* entities, int n_points) {
	int i;

	int i_min = -1;
	double r_min = 1;

	for(i = 0; i < n_points; i++) {
		double r = vector_dividing_ratio(start, stop, &(entities[i].pos));
		if (r > 0 && r < 1) {
			double d = vector_dist_to_line(start, stop, &(entities[i].pos));
			if (fabs(d) < entities[i].radius) {
				if(fabs(r-0.5) < fabs(r_min-0.5)) {
					i_min = i;
					r_min = r;
				}
			}
		}
	}

	if(i_min >= 0) {
		waypoint_t* wp = go_around(start, stop, &(entities[i_min]), r_min);

		waypoint_t* part1 = route(start, &(wp->point), entities, n_points);
		if(part1 == NULL) {
			part1 = wp;
		} else {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = wp;
		}
		waypoint_t* part2 = route(&(wp->point), stop, entities, n_points);
		if(part2 != 0) {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = part2;
		}
		return part1;
	} else {
		return NULL;
	}
}

waypoint_t* plotCourse(vector_t* start, vector_t* stop, entity_t* entities, int n) {
	waypoint_t* wp_start = malloc(sizeof(waypoint_t));
	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	wp_start->point = *start;
	wp_start->type = WP_START;
	wp_stop->point = *stop;
	wp_stop->type = WP_STOP;
	wp_start->next = route(start, stop, entities, n * n);
	wp_stop->next = NULL;
	waypoint_t* t = wp_start;

	while (t->next != NULL) {
		t = t->next;
	}

	t->next = wp_stop;
	return wp_start;
}

// To be called from Lua code
void moveto_planner(entity_t* e, double x, double y, char* callback) {
	if(e->type != SHIP) {
		fprintf(stderr, "We don't do moveto planing for non-ship entities\n");
		exit(1);
	}
	if(e->ship_data->flightplan != NULL) {
		free_route(e->ship_data->flightplan);
		e->ship_data->flightplan = NULL;
	}
	vector_t start;
	start.x = e->pos.x;
	start.y = e->pos.y;
	vector_t stop;
	stop.x = x;
	stop.y = y;

	waypoint_t* wp_start = malloc(sizeof(waypoint_t));
	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	wp_start->point = start;
	wp_start->type = WP_START;
	wp_start->next = wp_stop;
	wp_stop->point = stop;
	wp_stop->type = WP_STOP;
	wp_stop->next = NULL;

	e->ship_data->flightplan = wp_start;
	complete_flightplan(e);
}

// To be called from Lua code
void autopilot_planner(entity_t* e, double x, double y, char* callback) {
	if(e->type != SHIP) {
		fprintf(stderr, "We don't do autoroute planing for non-ship entities\n");
		exit(1);
	}
	if(e->ship_data->flightplan != NULL) {
		free_route(e->ship_data->flightplan);
		e->ship_data->flightplan = NULL;
	}
	vector_t start;
	start.x = e->pos.x;
	start.y = e->pos.y;
	vector_t stop;
	stop.x = x;
	stop.y = y;
	e->ship_data->flightplan = plotCourse(&start, &stop, map.cluster, map.clusters_x * map.clusters_y);
	complete_flightplan(e);
}
