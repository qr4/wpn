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
extern double MAXIMUM_PLANET_SIZE;
extern double MAXIMUM_CLUSTER_SIZE;

// Find a waypoint for the way between A and B avoiding C
waypoint_t* go_around(vector_t* A, vector_t* B, entity_t* C, double r) {
	vector_t X;
	X.v = A->v + (v2d) {r, r} * (B->v - A->v);
	double d = vector_dist(&X, &(C->pos));
	if(d < 1e-10) {
		fprintf(stderr, "A = (%f, %f), B = (%f, %f), C = (%f, %f), d = %f\n", A->x, A->y, B->x, B->y, C->pos.x, C->pos.y, d);
	}
	vector_t W;
	W.v = C->pos.v + (X.v - C->pos.v) * (v2d) {1.01, 1.01} * (v2d) {C->radius,  C->radius} / (v2d) {d, d};
	waypoint_t* wp = create_waypoint(W.x, W.y, 0, 0, 0, WP_TURN_VIA);
	wp->obs = C->pos;
	wp->swingbydist = 1.01 * C->radius;
	return wp;
}

waypoint_t* _route(vector_t* start, vector_t* stop, int level) {
	int i_min = -1;
	double r_min = 1;

	if(level > 30) {
		exit(1);
	}

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
		if(level > 20) {
			fprintf(stderr, "go around start = (%f, %f), stop = (%f, %f), cluster = (%f, %f) radius = %f, r = %f\n", start->x, start->y, stop->x, stop->y, map.cluster[i_min].pos.x, map.cluster[i_min].pos.y, map.cluster[i_min].radius, r_min);
		}
		waypoint_t* wp = go_around(start, stop, map.cluster + i_min, r_min);
		if(level > 20) {
			fprintf(stderr, "wp = (%f, %f) dist = %f\n", wp->point.x, wp->point.y, hypot(wp->point.x-map.cluster[i_min].pos.x, wp->point.y-map.cluster[i_min].pos.y));
		}

		waypoint_t* part1 = _route(start, &(wp->point), level+1);
		if(part1 == NULL) {
			part1 = wp;
		} else {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = wp;
		}
		waypoint_t* part2 = _route(&(wp->point), stop, level+1);
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

waypoint_t* route(vector_t* start, vector_t* stop) {
	return _route(start, stop, 0);
}

waypoint_t* _intra_cluster_route(vector_t* start, vector_t* stop, entity_t* cluster, int level) {
	if(cluster->cluster_data->asteroids == 0) {
		if(level > 10) {
			fprintf(stderr, "We are 10 deep in a single planet cluster\n");
			fprintf(stderr, "start = (%f, %f), stop = (%f, %f)\n", start->x, start->y, stop->x, stop->y);
			exit(1);
		}
		// Single planet in cluster
		entity_t* planet = get_entity_from_storage_by_id(planet_storage, cluster->cluster_data->planet);
		double r = vector_dividing_ratio(start, stop, &(planet->pos));
		if (r > 0 && r < 1) {
			double d = vector_dist_to_line(start, stop, &(planet->pos));
			if (fabs(d) < planet->radius) {
				waypoint_t* wp = go_around(start, stop, planet, r);

				waypoint_t* part1 = _intra_cluster_route(start, &(wp->point), cluster, level+1);
				if(part1 == NULL) {
					part1 = wp;
				} else {
					waypoint_t* t = part1;
					while(t->next != NULL) {
							t = t->next;
					}
					t->next = wp;
				}
				waypoint_t* part2 = _intra_cluster_route(&(wp->point), stop, cluster, level+1);
				if(part2 != NULL) {
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
		if(level > 20) {
			fprintf(stderr, "We are 20 deep in an astroid cluster\n");
			fprintf(stderr, "start = (%f, %f), stop = (%f, %f)\n", start->x, start->y, stop->x, stop->y);
			exit(1);
		}
		// Routing in astroid cluster
		int i_min = -1;
		double r_min = 1;
		for(int i = 0; i < cluster->cluster_data->asteroids; i++) {
			entity_t* asteroid = get_entity_from_storage_by_id(asteroid_storage, cluster->cluster_data->asteroid[i]);
			double r = vector_dividing_ratio(start, stop, &(asteroid->pos));
			if (r > 0 && r < 1) {
				double d = vector_dist_to_line(start, stop, &(asteroid->pos));
				if (fabs(d) < asteroid->radius) {
					if(fabs(r-0.5) < fabs(r_min-0.5)) {
						i_min = i;
						r_min = r;
					}
				}
			}
		}

		if(i_min >= 0) {
			entity_t* asteroid = get_entity_from_storage_by_id(asteroid_storage, cluster->cluster_data->asteroid[i_min]);
			if(level > 7) {
				fprintf(stderr, "go_around A = (%f, %f), B = (%f, %f), C = (%f, %f) radius=%f, r = %f\n", start->x, start->y, stop->x, stop->y, asteroid->pos.x, asteroid->pos.y, asteroid->radius, r_min);
			}
			waypoint_t* wp = go_around(start, stop, asteroid, r_min);

			waypoint_t* part1 = _intra_cluster_route(start, &(wp->point), cluster, level+1);
			if(part1 == NULL) {
				part1 = wp;
			} else {
				waypoint_t* t = part1;
				while(t->next != NULL) {
					t = t->next;
				}
				t->next = wp;
			}
			waypoint_t* part2 = _intra_cluster_route(&(wp->point), stop, cluster, level+1);
			if(part2 != NULL) {
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

waypoint_t* intra_cluster_route(vector_t* start, vector_t* stop, entity_t* cluster) {
	return _intra_cluster_route(start, stop, cluster, 0);
}

waypoint_t* plotCourse(vector_t* start, vector_t* stop) {
	waypoint_t* wp_start = create_waypoint(start->x, start->y, 0, 0, 0, WP_START);
	waypoint_t* wp_stop = create_waypoint(stop->x, stop->y, 0, 0, 0, WP_STOP);

	waypoint_t* jp1 = wp_start;
	waypoint_t* jp2 = wp_stop;

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
			jp1 = create_waypoint(0, 0, 0, 0, 0, WP_VIA);
			double dist = vector_dist(&(e_start->pos), start);
			if(dist == 0) { // Start at the center of a cluster
				double dir = 2 * M_PI * drand48();
				jp1->point.x = e_start->pos.x + sin(dir) * e_start->radius;
				jp1->point.y = e_start->pos.y + cos(dir) * e_start->radius;
			} else {
				jp1->point.x = e_start->pos.x + (start->x - e_start->pos.x) * e_start->radius / dist;
				jp1->point.y = e_start->pos.y + (start->y - e_start->pos.y) * e_start->radius / dist;
			}
			wp_start->next = intra_cluster_route(start, &(jp1->point), e_start);
		}
		if(e_stop != NULL) {
			// We stop in a cluster
			jp2 = create_waypoint(0, 0, 0, 0, 0, WP_VIA);
			double dist = vector_dist(&(e_stop->pos), stop);
			if(dist == 0) {
				double dir = 2 * M_PI * drand48();
				jp2->point.x = e_stop->pos.x + sin(dir) * e_stop->radius;
				jp2->point.y = e_stop->pos.y + cos(dir) * e_stop->radius;
			} else {
				jp2->point.x = e_stop->pos.x + (stop->x - e_stop->pos.x) * e_stop->radius / dist;
				jp2->point.y = e_stop->pos.y + (stop->y - e_stop->pos.y) * e_stop->radius / dist;
			}
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

	waypoint_t* wp_start = create_waypoint(start.x, start.y, 0, 0, 0, WP_START);
	waypoint_t* wp_stop = create_waypoint(stop.x, stop.y, 0, 0, 0, WP_STOP);
	wp_start->next = wp_stop;

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

		waypoint_t* wp_start = create_waypoint(start.x, start.y, 0, 0, 0, WP_START);
		waypoint_t* wp_stop = create_waypoint(stop.x, stop.y, 0, 0, 0, WP_STOP);
		wp_start->next = wp_stop;

		e->ship_data->flightplan = wp_start;
		complete_flightplan(e);
	} else {
		// Oh boy you are so fucked
		waypoint_t* wp_start = create_waypoint(start.x, start.y, 0, 0, 0, WP_VIA);
		waypoint_t* wp_stop = wp_start;

		vector_t end_pos;
		end_pos.v = start.v;
		for(int i = 0; i < 10000; i++) {
			end_pos.x += vel.x * dt;
			end_pos.y += vel.y * dt;

			waypoint_t* wp_tmp = create_waypoint(end_pos.x, end_pos.y, 0, 0, 0, WP_VIA);
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
	if(e->pos.x == x && e->pos.y == y) {
		// Already there
		fprintf(stderr, "We are already there\n");
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

	entity_t* se = find_closest_by_position(stop, 0, 2*MAXIMUM_PLANET_SIZE, ASTEROID|PLANET);
	if(se) {
		double dist = hypot(x - se->pos.x, y - se->pos.y);
		if(dist < (se->radius+1)) {
			fprintf(stderr, "This move brings you within %f of an object\n", hypot(x - se->pos.x, y - se->pos.y));
			double flight_dist = hypot(start.x - stop.x, start.y - stop.y);
			double r = (flight_dist - (se->radius+2)) / flight_dist;
			stop.x = start.x + r * (stop.x - start.x);
			stop.y = start.y + r * (stop.y - start.y);
			fprintf(stderr, "Stopping early at (%f, %f), dist = %f\n", stop.x, stop.y, hypot(stop.x - se->pos.x, stop.y - se->pos.y));
		}
	}

	e->ship_data->flightplan = plotCourse(&start, &stop);
	complete_flightplan(e);
}
