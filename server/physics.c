#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "physics.h"
#include "types.h"
#include "vector.h"

extern double dt;
extern double vmax;
extern double m0_small;
extern double m0_medium;
extern double m0_large;
extern double m0_huge;
extern double m0_klotz;
extern double F_thruster;
extern double epsilon;

// Berechnet die Masse eines Schiffes aus der Leermasse und der Anzahl der nichtleeren Klötze
double get_mass(entity_t* s) {
	double m = 0;

	if(!s) {
		fprintf(stderr, "Very funny, calling get_mass with s = NULL\n");
		exit(1);
	} else if(s->type != SHIP) {
		fprintf(stderr, "Very funny, calling get_mass with something that is not a ship\n");
		exit(1);
	}
	if(s->slots < 1) {
		fprintf(stderr, "Very funny, calling get_mass with something that has no slots\n");
		exit(1);
	} else if(s->slots <= 3) {
		m += m0_small;
	} else if(s->slots <= 6) {
		m += m0_medium;
	} else if(s->slots <= 12) {
		m += m0_large;
	} else if(s->slots <= 24) {
		m += m0_huge;
	} else {
		fprintf(stderr, "get_mass finds your death start very funny. not.\n");
		exit(1);
	}

	for(int i = 0; i < s->slots; i++) {
		if(s->ship_data->slot[i] != EMPTY) {
			m += m0_klotz;
		}
	}

	return m;
}

// Berechnet den Schub eines Schiffes aus der Anzahl der thruster
double get_thrust(entity_t* s) {
	double F = 0;

	if(!s) {
		fprintf(stderr, "Very funny, calling get_thrust with s = NULL\n");
		exit(1);
	} else if(s->type != SHIP) {
		fprintf(stderr, "Very funny, calling get_thrust with something that is not a ship\n");
		exit(1);
	} else if(s->slots < 1) {
		fprintf(stderr, "Very funny, calling get_thrust with s->slots = %d\n", s->slots);
		exit(1);
	}

	for(int i = 0; i < s->slots; i++) {
		if(s->ship_data->slot[i] == DRIVE) {
			F += F_thruster;
		}
	}

	return F;
}

// Berechnet die Maximalbeschleunigung eines Schiffes
double get_acceleration(entity_t* s) {
	if(!s) {
		fprintf(stderr, "Very funny, calling get_acceleration with s = NULL\n");
		exit(1);
	}
	return get_thrust(s) / get_mass(s);
}

double get_max_curve_speed(entity_t* s, double r) {
	double speed = sqrt(get_acceleration(s) * r);
	if (speed > vmax) {
		speed = vmax;
	}
	return speed;
}

waypoint_t* create_waypoint(double x, double y, double vx, double vy, double t, wptype type) {
	waypoint_t* ret = malloc(sizeof(waypoint_t));
	if(ret == NULL) {
		fprintf(stderr, "No memory left for another waypoint. Most likely you are leaking them and need to debug your program\n");
		exit(1);
	}
	ret->point.x = x;
	ret->point.y = y;
	ret->speed.x = vx;
	ret->speed.y = vy;
	ret->t = t;
	ret->type = type;
	ret->next = NULL;

	return ret;
}

void free_waypoint(waypoint_t* wp) {
	free(wp);
}

void free_route(waypoint_t* head) {
	while(head != NULL) {
		waypoint_t* next = head->next;
		free_waypoint(head);
		head = next;
	}
}

void fprint_waypoint(FILE *stream, waypoint_t* wp) {
	if(wp != NULL) {
		char* type;
		switch(wp->type) {
			case WP_START:
				type = "start";
				break;
			case WP_TURN_START:
				type = "start_turn";
				break;
			case WP_TURN_VIA:
				type = "turn_via";
				break;
			case WP_TURN_STOP:
				type = "stop_turn";
				break;
			case WP_VIA:
				type = "via";
				break;
			case WP_STOP:
				type = "stop";
				break;
			default:
				type = "unknown";
		}
		fprintf(stream, "t=%f, x=(%f,%f), v=(%f,%f) [%s]\n", wp->t, wp->point.x, wp->point.y, wp->speed.x, wp->speed.y, type);
		if(wp->type == WP_TURN_START || wp->type == WP_TURN_VIA || wp->type == WP_TURN_STOP) {
			fprintf(stream, "    (trying to avoid (%f, %f) by %f\n", wp->obs.x, wp->obs.y, wp->swingbydist);
		}
	}
}

void print_waypoint(waypoint_t* wp) {
	fprint_waypoint(stdout, wp);
}

void print_route(waypoint_t* head) {
	while(head) {
		print_waypoint(head);
		head = head->next;
	}
}

double dist_to_vmax(double v0, double a) {
	// If v0 is negative the distance is actually the same, but the travel time
	// is longer. If that happens we are fucked anyway.
	return (vmax*vmax - v0*v0)/(2 * a);
}

void coast(waypoint_t* wp0, waypoint_t* wp1) {
	if(wp0 == NULL) {
		fprintf(stderr, "Very funny, calling coast with wp0 == NULL\n");
		return;
	} else if(wp1 == NULL) {
		fprintf(stderr, "Very funny, calling coast with wp1 == NULL\n");
		return;
	} else if((fabs(wp0->speed.x - wp1->speed.x) > epsilon) || (fabs(wp0->speed.y - wp1->speed.y) > epsilon)) {
		fprintf(stderr, "Coasting wont magically take you from v0 = (%f,%f) to v1 = (%f,%f)\n", wp0->speed.x, wp0->speed.y, wp1->speed.x, wp1->speed.y);
		return;
	}

	double dist = hypot(wp1->point.x - wp0->point.x, wp1->point.y - wp0->point.y);
	double t_coast = dist / hypot(wp0->speed.x, wp0->speed.y);

	waypoint_t* wpx = wp0;
	for(double t = ceil(wp0->t / dt)*dt; t < wp0->t + t_coast; t += dt) {
		double delta_t = t - wp0->t;
		waypoint_t* wpy = create_waypoint(wp0->point.x + wp0->speed.x * delta_t, wp0->point.y + wp0->speed.y * delta_t, wp0->speed.x, wp0->speed.y, t, WP_VIA);
		wpx->next = wpy;
		wpx = wpy;
	}
	wpx->next = wp1;
	wp1->t = wp0->t + t_coast;
}

void _straight_minimal_time(waypoint_t* wp0, waypoint_t* wp1, double a) {
	if(wp0 == NULL) {
		fprintf(stderr, "Very funny, calling _straight_minimal_time with wp0 == NULL\n");
		return;
	} else if(wp1 == NULL) {
		fprintf(stderr, "Very funny, calling _straight_minimal_time with wp1 == NULL\n");
		return;
	}

	double dist = hypot(wp1->point.x - wp0->point.x, wp1->point.y - wp0->point.y);
	double distx = wp1->point.x - wp0->point.x;
	double disty = wp1->point.y - wp0->point.y;
	double v0 = hypot(wp0->speed.x, wp0->speed.y);
	double v1 = hypot(wp1->speed.x, wp1->speed.y);
	// Calculated by matching two parabola from x0,v0 to x1,v1 to find the
	// midway time where position and speed match. That would be the point
	// where a real spacecarft would have to turn around and start to apply
	// reverse thrust. det is the determinant of the quadratic equation.
	double det = sqrt(2*(v0 * v0 + v1 * v1 + 2 * a * dist));
	double t_midway = (0.5 * det - v0) / a;
	double t_end = (det - v0 - v1) / a;

	if(t_midway > -epsilon && t_end > -epsilon && t_end - t_midway > -epsilon) { // Manchmal hasse ich Fließkommagenauigkeit
		waypoint_t* wpx = wp0;
		for(double t = ceil(wp0->t / dt)*dt; t < wp0->t + t_end; t += dt) {
			if(t <= wp0->t + t_midway) {
				double delta_t = t - wp0->t;
				waypoint_t* wpy = create_waypoint(wp0->point.x + wp0->speed.x * delta_t + 0.5 * a * distx * delta_t * delta_t / dist, wp0->point.y + wp0->speed.y * delta_t + 0.5 * a * disty * delta_t * delta_t / dist, wp0->speed.x + a * distx * delta_t / dist, wp0->speed.y + a * disty * delta_t / dist, t, WP_VIA);
				wpx->next = wpy;
				wpx = wpy;
				wpx = wpy;
			} else {
				double delta_t = t - wp0->t - t_end;
				waypoint_t* wpy = create_waypoint(wp1->point.x + wp1->speed.x * delta_t - 0.5 * a * delta_t * delta_t * distx / dist, wp1->point.y + wp1->speed.y * delta_t - 0.5 * a * delta_t * delta_t * disty / dist, wp1->speed.x - a * delta_t * distx / dist, wp1->speed.y - a * delta_t * disty / dist, t, WP_VIA);
				wpx->next = wpy;
				wpx = wpy;
			}
		}
		wpx->next = wp1;
		wp1->t = wp0->t + t_end;
	} else {
		fprintf(stderr, "Sorry: t_midway = %e and t_end = %e (difference %e) mean that _straight_minimal_time aint't working\n", t_midway, t_end, t_end - t_midway);
		fprintf(stderr, "Start:\n");
		fprint_waypoint(stderr, wp0);
		fprintf(stderr, "Stop: \n");
		fprint_waypoint(stderr, wp1);
		fprintf(stderr, "a = %f\n", a);
		return;
	}
}

void straight_minimal_time(waypoint_t* wp0, waypoint_t* wp1, double a) {
	if(wp0 == NULL) {
		fprintf(stderr, "Very funny, calling straight_minimal_time with wp0 == NULL\n");
		return;
	} else if(wp1 == NULL) {
		fprintf(stderr, "Very funny, calling straight_minimal_time with wp1 == NULL\n");
		return;
	} else if(isnan(wp0->point.x)) {
		fprintf(stderr, "Very funny, calling straight_minimal_time with wp0.x == NaN\n");
		return;
	} else if(isnan(wp0->point.y)) {
		fprintf(stderr, "Very funny, calling straight_minimal_time with wp0.y == NaN\n");
		return;
	} else if(isnan(wp1->point.x)) {
		fprintf(stderr, "Very funny, calling straight_minimal_time with wp1.x == NaN\n");
		return;
	} else if(isnan(wp1->point.y)) {
		fprintf(stderr, "Very funny, calling straight_minimal_time with wp1.y == NaN\n");
		return;
	}


	double l_spinup = dist_to_vmax(hypot(wp0->speed.x, wp0->speed.y), a);
	double l_spindown = dist_to_vmax(hypot(wp1->speed.x, wp1->speed.y), a);
	double dist = hypot(wp1->point.x - wp0->point.x, wp1->point.y - wp0->point.y);
	double distx = wp1->point.x - wp0->point.x;
	double disty = wp1->point.y - wp0->point.y;

	if(dist < l_spinup + l_spindown) {
		_straight_minimal_time(wp0, wp1, a);
	} else {
		waypoint_t* wp_coaststart = create_waypoint(wp0->point.x + l_spinup*distx/dist, wp0->point.y + l_spinup*disty/dist, vmax*distx/dist, vmax*disty/dist, 0, WP_VIA);
		_straight_minimal_time(wp0, wp_coaststart, a);
		waypoint_t* wp_coaststop = create_waypoint(wp1->point.x - l_spindown*distx/dist, wp1->point.y - l_spindown*disty/dist, vmax*distx/dist, vmax*disty/dist, 0, WP_VIA);
		coast(wp_coaststart, wp_coaststop);
		_straight_minimal_time(wp_coaststop, wp1, a);
	}
}

void swing_by(waypoint_t* wp0, vector_t obstacle, waypoint_t* wp1, double a) {
	if(wp0 == NULL) {
		fprintf(stderr, "Very funny, calling swing_by with wp0 == NULL\n");
		return;
	} else if(wp1 == NULL) {
		fprintf(stderr, "Very funny, calling swing_by with wp1 == NULL\n");
		return;
	}

	double vdiff = hypot(wp0->speed.x, wp0->speed.y) - hypot(wp1->speed.x, wp1->speed.y);
	if(vdiff > 2*epsilon) { // Da quadratisch im Fehler
		fprintf(stderr, "A swing_by wont magically take you from v0 = (%f,%f) to v1 = (%f,%f) (diff = %e)\n", wp0->speed.x, wp0->speed.y, wp1->speed.x, wp1->speed.y, vdiff);
		return;
	}

	double distx = wp0->point.x - obstacle.x;
	double disty = wp0->point.y - obstacle.y;
	double dist0 = hypot(distx, disty);
	double dist1 = hypot(wp1->point.x - obstacle.x, wp1->point.y - obstacle.y);

	// clockwise is positive
	double turn_rate = (disty * wp0->speed.x - distx * wp0->speed.y) / (dist0 * dist0);
	double bearing0 = atan2(distx, disty);
	double bearing1 = atan2(wp1->point.x - obstacle.x, wp1->point.y - obstacle.y);

	double angle = bearing1 - bearing0;
	double t_rotate = angle / turn_rate;

	if(fabs(dist0 - dist1) > epsilon) {
		fprintf(stderr, "A swing_by can't just change the distance from %f to %f (obs = %f, %f, diff = %e)\n", dist0, dist1, obstacle.x, obstacle.y, dist0 - dist1);
		fprint_waypoint(stderr, wp0);
		fprint_waypoint(stderr, wp1);
		return;
	}

	waypoint_t* wpx = wp0;
	for(double t = ceil(wp0->t / dt)*dt; t < wp0->t + t_rotate; t += dt) {
		double delta_t = t - wp0->t;
		double s = sin(turn_rate*delta_t);
		double c = cos(turn_rate*delta_t);

		double newx = obstacle.x + distx * c + disty * s;
		double newy = obstacle.y - distx * s + disty * c;
		double newvx =  wp0->speed.x * c + wp0->speed.y * s;
		double newvy = -wp0->speed.x * s + wp0->speed.y * c;

		waypoint_t* wpy = create_waypoint(newx, newy, newvx, newvy, t, WP_TURN_VIA);
		wpx->next = wpy;
		wpx = wpy;
		wpx = wpy;
	}
	wpx->next = wp1;
	wp1->t = wp0->t + t_rotate;
}

vector_t point_tangent(vector_t p, vector_t center, double r, vector_t hint) {
	double x0 = center.x - p.x;
	double y0 = center.y - p.y;

	double t0 =  acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
	double t1 =  acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
	double t2 = -acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
	double t3 = -acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));

	double d0 = hypot(p.x + x0 + r * cos(t0) - hint.x, p.y + y0 + r * sin(t0) - hint.y);
	double d1 = hypot(p.x + x0 + r * cos(t1) - hint.x, p.y + y0 + r * sin(t1) - hint.y);
	double d2 = hypot(p.x + x0 + r * cos(t2) - hint.x, p.y + y0 + r * sin(t2) - hint.y);
	double d3 = hypot(p.x + x0 + r * cos(t3) - hint.x, p.y + y0 + r * sin(t3) - hint.y);

	vector_t ret;
	if(d0 <= d1 && d0 <= d2 && d0 <= d3) {
		ret.x = p.x + x0 + r * cos(t0);
		ret.y = p.y + y0 + r * sin(t0);
	} else if(d1 <= d0 && d1 <= d2 && d1 <= d3) {
		ret.x = p.x + x0 + r * cos(t1);
		ret.y = p.y + y0 + r * sin(t1);
	} else if(d2 <= d0 && d2 <= d1 && d2 <= d3) {
		ret.x = p.x + x0 + r * cos(t2);
		ret.y = p.y + y0 + r * sin(t2);
	} else {
		ret.x = p.x + x0 + r * cos(t3);
		ret.y = p.y + y0 + r * sin(t3);
	}
	return ret;
}

// Aussentangente zwischen den beiden kreisen, die möglichst nahe an hint ist
vector_t outer_tangent(vector_t c1, double r1, vector_t hint1, vector_t c2, double r2, vector_t hint2) {
	if(r1 < r2) {
		double r = r2-r1;
		double x0 = c2.x - c1.x;
		double y0 = c2.y - c1.y;

		double t0 =  acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
		double t1 =  acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
		double t2 = -acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
		double t3 = -acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));

		vector_t ta0;
		ta0.x = c1.x + x0 + r2 * cos(t0);
		ta0.y = c1.y + y0 + r2 * sin(t0);
		vector_t ta1;
		ta1.x = c1.x + x0 + r2 * cos(t1);
		ta1.y = c1.y + y0 + r2 * sin(t1);
		vector_t ta2;
		ta2.x = c1.x + x0 + r2 * cos(t2);
		ta2.y = c1.y + y0 + r2 * sin(t2);
		vector_t ta3;
		ta3.x = c1.x + x0 + r2 * cos(t3);
		ta3.y = c1.y + y0 + r2 * sin(t3);

		double d0 = hypot(ta0.x - hint2.x, ta0.y - hint2.y);
		double d1 = hypot(ta1.x - hint2.x, ta1.y - hint2.y);
		double d2 = hypot(ta2.x - hint2.x, ta2.y - hint2.y);
		double d3 = hypot(ta3.x - hint2.x, ta3.y - hint2.y);

		vector_t ret;
		if(d0 <= d1 && d0 <= d2 && d0 <= d3) {
			ret.x = c1.x + r1 * cos(t0);
			ret.y = c1.y + r1 * sin(t0);
		} else if(d1 <= d0 && d1 <= d2 && d1 <= d3) {
			ret.x = c1.x + r1 * cos(t1);
			ret.y = c1.y + r1 * sin(t1);
		} else if(d2 <= d0 && d2 <= d1 && d2 <= d3) {
			ret.x = c1.x + r1 * cos(t2);
			ret.y = c1.y + r1 * sin(t2);
		} else {
			ret.x = c1.x + r1 * cos(t3);
			ret.y = c1.y + r1 * sin(t3);
		}
		return ret;
	} else { // r1 > r2
		double r = r1-r2;
		double x0 = c1.x - c2.x;
		double y0 = c1.y - c2.y;

		double t0 =  acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
		double t1 =  acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
		double t2 = -acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
		double t3 = -acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));

		vector_t ta0;
		ta0.x = c1.x + r1 * cos(t0);
		ta0.y = c1.y + r1 * sin(t0);
		vector_t ta1;
		ta1.x = c1.x + r1 * cos(t1);
		ta1.y = c1.y + r1 * sin(t1);
		vector_t ta2;
		ta2.x = c1.x + r1 * cos(t2);
		ta2.y = c1.y + r1 * sin(t2);
		vector_t ta3;
		ta3.x = c1.x + r1 * cos(t3);
		ta3.y = c1.y + r1 * sin(t3);

		double d0 = hypot(ta0.x - hint1.x, ta0.y - hint1.y);
		double d1 = hypot(ta1.x - hint1.x, ta1.y - hint1.y);
		double d2 = hypot(ta2.x - hint1.x, ta2.y - hint1.y);
		double d3 = hypot(ta3.x - hint1.x, ta3.y - hint1.y);

		if(d0 <= d1 && d0 <= d2 && d0 <= d3) {
			return ta0;
		} else if(d1 <= d0 && d1 <= d2 && d1 <= d3) {
			return ta1;
		} else if(d2 <= d0 && d2 <= d1 && d2 <= d3) {
			return ta2;
		} else {
			return ta3;
		}
	}
}

// Innentangente zwischen den beiden kreisen, die möglichst nahe an hint ist
vector_t inner_tangent(vector_t c1, double r1, vector_t hint1, vector_t c2, double r2, vector_t hint2) {
	double r = r1+r2;
	double x0 = c1.x - c2.x;
	double y0 = c1.y - c2.y;

	double t0 =  acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
	double t1 =  acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
	double t2 = -acos((-r * x0 + y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));
	double t3 = -acos((-r * x0 - y0 * sqrt(x0*x0 + y0*y0 - r*r)) / (x0*x0 + y0*y0));

	vector_t ta0;
	ta0.x = c1.x + r1 * cos(t0);
	ta0.y = c1.y + r1 * sin(t0);
	vector_t ta1;
	ta1.x = c1.x + r1 * cos(t1);
	ta1.y = c1.y + r1 * sin(t1);
	vector_t ta2;
	ta2.x = c1.x + r1 * cos(t2);
	ta2.y = c1.y + r1 * sin(t2);
	vector_t ta3;
	ta3.x = c1.x + r1 * cos(t3);
	ta3.y = c1.y + r1 * sin(t3);

	double d0 = hypot(ta0.x - hint1.x, ta0.y - hint1.y);
	double d1 = hypot(ta1.x - hint1.x, ta1.y - hint1.y);
	double d2 = hypot(ta2.x - hint1.x, ta2.y - hint1.y);
	double d3 = hypot(ta3.x - hint1.x, ta3.y - hint1.y);

	if(d0 <= d1 && d0 <= d2 && d0 <= d3) {
		return ta0;
	} else if(d1 <= d0 && d1 <= d2 && d1 <= d3) {
		return ta1;
	} else if(d2 <= d0 && d2 <= d1 && d2 <= d3) {
		return ta2;
	} else {
		return ta3;
	}
}

char equals(vector_t A, vector_t B) {
	return A.x == B.x && A.y == B.y;
}

char equals3(vector_t A, vector_t B, vector_t C) {
	return A.x == B.x && B.x == C.x && A.y == B.y && B.y == C.y;
}

void splice_curves(entity_t* s) {
	// Insert tangent at start and stop
	waypoint_t* prev = s->ship_data->flightplan;
	waypoint_t* via = s->ship_data->flightplan->next;
	while(prev && via) {
		if(prev && via && prev->type == WP_START && via->type == WP_TURN_VIA) {
			vector_t tangent_start = point_tangent(prev->point, via->obs, hypot(via->obs.x - via->point.x, via->obs.y - via->point.y), via->point);
			double r = hypot(tangent_start.x - via->obs.x, tangent_start.y - via->obs.y);
			double v = get_max_curve_speed(s, r);
			double vx = -v * (tangent_start.y - via->obs.y) / r;
			double vy = v * (tangent_start.x - via->obs.x) / r;
			waypoint_t* t = create_waypoint(tangent_start.x, tangent_start.y, vx, vy, 0, WP_TURN_START);
			t->obs.x = via->obs.x;
			t->obs.y = via->obs.y;
			prev->next = t;
			t->next = via;
		}
		if(prev && via && prev->type == WP_TURN_VIA && via->type == WP_STOP) {
			vector_t tangent_stop = point_tangent(via->point, prev->obs, hypot(prev->obs.x - prev->point.x, prev->obs.y - prev->point.y), prev->point);
			double r = hypot(tangent_stop.x - prev->obs.x, tangent_stop.y - prev->obs.y);
			double v = get_max_curve_speed(s, r);
			double vx = -v * (tangent_stop.y - prev->obs.y) / r;
			double vy = v * (tangent_stop.x - prev->obs.x) / r;
			waypoint_t* t = create_waypoint(tangent_stop.x, tangent_stop.y, vx, vy, 0, WP_TURN_STOP);
			t->obs.x = prev->obs.x;
			t->obs.y = prev->obs.y;
			prev->next = t;
			t->next = via;
		}

		prev = via;
		via = via->next;
	}

	// Remove redundant WP_TURN_VIA nodes
	prev = s->ship_data->flightplan;
	via = s->ship_data->flightplan->next;
	while(prev && via) {
		waypoint_t* next = via->next;
		if((prev->type == WP_TURN_START || prev->type == WP_TURN_VIA) && via->type == WP_TURN_VIA && (next->type == WP_TURN_VIA || next->type == WP_TURN_STOP) && equals3(prev->obs, via->obs, next->obs)) {
			free_waypoint(via);
			prev->next = next;
			via = next;
		} else {
			prev = via;
			via = next;
		}
	}

	// Insert tangents
	prev = s->ship_data->flightplan;
	via = s->ship_data->flightplan->next;
	while(prev && via) {
		if(prev->type == WP_TURN_VIA && via->type == WP_TURN_VIA && !equals(prev->obs, via->obs)) {
			double dist1 = vector_dist_to_line(prev->obs, via->obs, prev->point);
			double dist2 = vector_dist_to_line(prev->obs, via->obs, via->point);
			double r1 = hypot(prev->point.x - prev->obs.x, prev->point.y - prev->obs.y);
			double r2 = hypot(via->point.x - via->obs.x, via->point.y - via->obs.y);
			if(dist1 * dist2 > 0) { // Aussentangente
				vector_t t1 = outer_tangent(prev->obs, r1, prev->point, via->obs, r2, via->point);
				double vmax1 = get_max_curve_speed(s, r1);
				double vx1 = -vmax1 * (t1.y - prev->obs.y) / hypot(t1.x - prev->obs.x, t1.y - prev->obs.y);
				double vy1 = vmax1 * (t1.x - prev->obs.x) / hypot(t1.x - prev->obs.x, t1.y - prev->obs.y);
				waypoint_t* wp1 = create_waypoint(t1.x, t1.y, vx1, vy1, 0, WP_TURN_STOP);
				wp1->obs.x = prev->obs.x;
				wp1->obs.y = prev->obs.y;

				vector_t t2;
				t2.x = via->obs.x + (t1.x - prev->obs.x) * r2 / r1;
				t2.y = via->obs.y + (t1.y - prev->obs.y) * r2 / r1;
				double vmax2 = get_max_curve_speed(s, r2);
				double vx2 = -vmax2 * (t2.y - via->obs.y) / hypot(t2.x - via->obs.x, t2.y - via->obs.y);
				double vy2 = vmax2 * (t2.x - via->obs.x) / hypot(t2.x - via->obs.x, t2.y - via->obs.y);
				waypoint_t* wp2 = create_waypoint(t2.x, t2.y, vx2, vy2, 0, WP_TURN_START);
				wp2->obs.x = via->obs.x;
				wp2->obs.y = via->obs.y;

				prev->next = wp1;
				wp1->next = wp2;
				wp2->next = via;
			} else { // Innentangente
				vector_t t1 = inner_tangent(prev->obs, r1, prev->point, via->obs, r2, via->point);
				double vmax1 = get_max_curve_speed(s, r1);
				double vx1 = -vmax1 * (t1.y - prev->obs.y) / hypot(t1.x - prev->obs.x, t1.y - prev->obs.y);
				double vy1 = vmax1 * (t1.x - prev->obs.x) / hypot(t1.x - prev->obs.x, t1.y - prev->obs.y);
				waypoint_t* wp1 = create_waypoint(t1.x, t1.y, vx1, vy1, 0, WP_TURN_STOP);
				wp1->obs.x = prev->obs.x;
				wp1->obs.y = prev->obs.y;

				vector_t t2;
				t2.x = via->obs.x - (t1.x - prev->obs.x) * r2 / r1;
				t2.y = via->obs.y - (t1.y - prev->obs.y) * r2 / r1;
				double vmax2 = get_max_curve_speed(s, r2);
				double vx2 = -vmax2 * (t2.y - via->obs.y) / hypot(t2.x - via->obs.x, t2.y - via->obs.y);
				double vy2 = vmax2 * (t2.x - via->obs.x) / hypot(t2.x - via->obs.x, t2.y - via->obs.y);
				waypoint_t* wp2 = create_waypoint(t2.x, t2.y, vx2, vy2, 0, WP_TURN_START);
				wp2->obs.x = via->obs.x;
				wp2->obs.y = via->obs.y;

				prev->next = wp1;
				wp1->next = wp2;
				wp2->next = via;
			}
		}
		prev = via;
		via = via->next;
	}

	// Remove remaining redundant WP_TURN_VIA nodes
	prev = s->ship_data->flightplan;
	via = s->ship_data->flightplan->next;
	while(prev && via) {
		waypoint_t* next = via->next;
		if((prev->type == WP_TURN_START || prev->type == WP_TURN_VIA) && via->type == WP_TURN_VIA && (next->type == WP_TURN_VIA || next->type == WP_TURN_STOP) && equals3(prev->obs, via->obs, next->obs)) {
			free_waypoint(via);
			prev->next = next;
			via = next;
		} else {
			prev = via;
			via = next;
		}
	}
}

void complete_flightplan(entity_t* s) {
	waypoint_t* prev = s->ship_data->flightplan;
	waypoint_t* via = s->ship_data->flightplan->next;
	double a = get_acceleration(s);

	while(prev && via) {
		if(prev->type == WP_TURN_START && via->type == WP_TURN_STOP) {
			swing_by(prev, prev->obs, via, a);
		} else {
			straight_minimal_time(prev, via, a);
		}
		prev = via;
		via = via->next;
	}
}
