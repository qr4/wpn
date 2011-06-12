#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "map.h"
#include "physics.h"
#include "route.h"
#include "types.h"
#include "entities.h"
#include "entity_storage.h"
#include "physics.h"
#include "storages.h"

extern entity_storage_t* asteroid_storage;
extern entity_storage_t* planet_storage;
extern map_t map; // cluster

extern double dt;

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

waypoint_t* route(vector_t* start, vector_t* stop) {
	int i_min = -1;
	double r_min = 1;

	for(int i = 0; i < map.clusters_x * map.clusters_y; i++) {
		double r = vector_dividing_ratio(start, stop, &(map.cluster[i].pos));
		if (r > 0 && r < 1) {
			double d = vector_dist_to_line(start, stop, &(map.cluster[i].pos));
			if (fabs(d) < map.cluster[i].radius) {
				if(fabs(r-0.5) < fabs(r_min-0.5)) {
					i_min = i;
					r_min = r;
				}
			}
		}
	}

	if(i_min >= 0) {
		waypoint_t* wp = go_around(start, stop, map.cluster + i_min, r_min);

		waypoint_t* part1 = route(start, &(wp->point));
		if(part1 == NULL) {
			part1 = wp;
		} else {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = wp;
		}
		waypoint_t* part2 = route(&(wp->point), stop);
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

waypoint_t* intra_cluster_route(vector_t* start, vector_t* stop, entity_t* cluster) {
	if(cluster->cluster_data->asteroids == 0) {
		// Single planet in cluster
		entity_t* planet = get_entity_from_storage_by_id(planet_storage, cluster->cluster_data->planet);
		double r = vector_dividing_ratio(start, stop, &(planet->pos));
		if (r > 0 && r < 1) {
			double d = vector_dist_to_line(start, stop, &(planet->pos));
			if (fabs(d) < planet->radius) {
				waypoint_t* wp = go_around(start, stop, planet, r);

				waypoint_t* part1 = intra_cluster_route(start, &(wp->point), cluster);
				if(part1 == NULL) {
					part1 = wp;
				} else {
					waypoint_t* t = part1;
					while(t->next != NULL) {
							t = t->next;
					}
					t->next = wp;
				}
				waypoint_t* part2 = intra_cluster_route(&(wp->point), stop, cluster);
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
		} else {
			return NULL;
		}
	} else {
		// Routing in astroid cluster
		int i_min = -1;
		double r_min = 1;
		for(int i = 0; i < cluster->cluster_data->asteroids; i++) {
			entity_t* asteroid = get_entity_from_storage_by_id(asteroid_storage, cluster->cluster_data->asteroid[i]);
			double r = vector_dividing_ratio(start, stop, &(asteroid->pos));
			if (r > 0 && r < 1) {
				double d = vector_dist_to_line(start, stop, &(asteroid->pos));
				if (fabs(d) < map.cluster[i].radius) {
					if(fabs(r-0.5) < fabs(r_min-0.5)) {
						i_min = i;
						r_min = r;
					}
				}
			}
		}

		if(i_min >= 0) {
			entity_t* asteroid = get_entity_from_storage_by_id(asteroid_storage, cluster->cluster_data->asteroid[i_min]);
			waypoint_t* wp = go_around(start, stop, asteroid, r_min);

			waypoint_t* part1 = intra_cluster_route(start, &(wp->point), cluster);
			if(part1 == NULL) {
				part1 = wp;
			} else {
				waypoint_t* t = part1;
				while(t->next != NULL) {
					t = t->next;
				}
				t->next = wp;
			}
			waypoint_t* part2 = intra_cluster_route(&(wp->point), stop, cluster);
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
}

waypoint_t* plotCourse(vector_t* start, vector_t* stop) {
	waypoint_t* wp_start = malloc(sizeof(waypoint_t));
	if(!wp_start) {
		fprintf(stderr, "No start no joy.\n");
		exit(1);
	}

	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	if(!wp_stop) {
		fprintf(stderr, "No stop no joy.\n");
		exit(1);
	}

	waypoint_t* jp1 = wp_start;
	waypoint_t* jp2 = wp_stop;

	wp_start->point = *start;
	wp_start->speed = (vector_t){{0, 0}};
	wp_start->t = 0;
	wp_start->type = WP_START;
	wp_start->next = NULL;
	wp_stop->point = *stop;
	wp_stop->speed = (vector_t){{0, 0}};
	wp_start->type = WP_START;
	wp_stop->type = WP_STOP;
	wp_stop->next = NULL;

	entity_t* e_start = find_closest_by_position(*start, 0, 0, CLUSTER);
	entity_t* e_stop = find_closest_by_position(*stop, 0, 0, CLUSTER);

	waypoint_t* t;

	if(e_start == NULL && e_stop == NULL) {
		// inter cluster flight
		wp_start->next = route(start, stop);
	} else if(e_start != NULL && e_stop != NULL && e_start->pos.x == e_stop->pos.x && e_start->pos.y == e_stop->pos.y) {
		// flight within a cluster
		wp_start->next = intra_cluster_route(start, stop, e_start);
	} else {
		if(e_start != NULL) {
			// We start in a cluster
			jp1 = malloc(sizeof(waypoint_t));
			if(!jp1) {
				fprintf(stderr, "No jp1, no joy\n");
			}
			jp1->point.x = e_start->pos.x + (start->x - e_start->pos.x) * e_start->radius / vector_dist(&(e_start->pos), start);
			jp1->point.y = e_start->pos.y + (start->y - e_start->pos.y) * e_start->radius / vector_dist(&(e_start->pos), start);
			jp1->type = WP_VIA;
			jp1->next = NULL;
			wp_start->next = intra_cluster_route(start, &(jp1->point), e_start);
		}
		if(e_stop != NULL) {
			// We start in a cluster
			jp2 = malloc(sizeof(waypoint_t));
			if(!jp2) {
				fprintf(stderr, "No jp1, no joy\n");
			}
			jp2->point.x = e_stop->pos.x + (stop->x - e_stop->pos.x) * e_stop->radius / vector_dist(&(e_stop->pos), stop);
			jp2->point.y = e_stop->pos.y + (stop->y - e_stop->pos.y) * e_stop->radius / vector_dist(&(e_stop->pos), stop);
			jp2->type = WP_VIA;
			jp2->next = intra_cluster_route(&(jp2->point), stop, e_stop);
			t = jp2;
			while (t->next != NULL) {
				t = t->next;
			}
			t->next = wp_stop;
		}

		route(&(jp1->point), &(jp2->point));
	}

	t = wp_start;
	while (t->next != NULL) {
		t = t->next;
	}
	t->next = jp2;
	return wp_start;
}

// To be called from Lua code
void moveto_planner(entity_t* e, double x, double y) {
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
	if(!wp_start) {
		fprintf(stderr, "No wp_start in moveto_planer\n");
		exit(1);
	}
	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	if(!wp_stop) {
		fprintf(stderr, "No wp_stop in moveto_planer\n");
		exit(1);
	}
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
void stop_planner(entity_t* e) {
	if(e->type != SHIP) {
		fprintf(stderr, "We don't do moveto planing for non-ship entities\n");
		exit(1);
	}
	if(e->ship_data->flightplan == NULL) {
		return;
	}

	vector_t start;
	start.v = e->pos.v;
	vector_t vel;
	vel.v = e->ship_data->flightplan->speed.v;
	double a = get_acceleration(e);

	free_route(e->ship_data->flightplan);
	e->ship_data->flightplan = NULL;

	if(a > 0) {
		double speed = hypot(vel.x, vel.y);
		double stopping_distance = speed/(2*a);
		vector_t stop;
		stop.x = start.x + stopping_distance * vel.x / speed;
		stop.y = start.y + stopping_distance * vel.y / speed;

		waypoint_t* wp_start = malloc(sizeof(waypoint_t));
		if(!wp_start) {
			fprintf(stderr, "No wp_start in stop_planner\n");
			exit(1);
		}
		waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
		if(!wp_stop) {
			fprintf(stderr, "No wp_stop in stop_planner\n");
			exit(1);
		}
		wp_start->point = start;
		wp_start->type = WP_START;
		wp_start->next = wp_stop;
		wp_stop->point = stop;
		wp_stop->type = WP_STOP;
		wp_stop->next = NULL;

		e->ship_data->flightplan = wp_start;
		complete_flightplan(e);
	} else {
		// Oh boy you are so fucked
		waypoint_t* wp_start = malloc(sizeof(waypoint_t));
		if(!wp_start) {
			fprintf(stderr, "No wp_start in stop_planner\n");
			exit(1);
		}
		waypoint_t* wp_stop = wp_start;
		wp_start->type = WP_VIA;
		wp_start->point = start;
		wp_start->next = NULL;

		vector_t end_pos;
		end_pos.v = start.v;
		for(int i = 0; i < 10000; i++) {
			end_pos.x += vel.x * dt;
			end_pos.y += vel.y * dt;

			waypoint_t* wp_tmp = malloc(sizeof(waypoint_t));
			if(!wp_tmp) {
				fprintf(stderr, "No wp_tmp in stop_planner\n");
				exit(1);
			}
			wp_tmp->point = end_pos;
			wp_tmp->type = WP_VIA;
			wp_tmp->next = NULL;
			wp_stop->next = wp_tmp;
			wp_stop = wp_tmp;

			if(find_closest_by_position(e->pos, e->radius, 100, ASTEROID|PLANET) < 0) {
				break;
			}
		}
		e->ship_data->flightplan = wp_start;
	}
}

// To be called from Lua code
void autopilot_planner(entity_t* e, double x, double y) {
	if(e->type != SHIP) {
		fprintf(stderr, "We don't do autoroute planing for non-ship entities\n");
		exit(1);
	}
	if(get_acceleration(e) == 0) {
		fprintf(stderr, "Flying without engines? Talk to Mr. Scott first.\n");
		return;
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
	e->ship_data->flightplan = plotCourse(&start, &stop);
	complete_flightplan(e);
}
