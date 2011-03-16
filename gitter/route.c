#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "route.h"
#include "globals.h"

// Abstand zwischen zwei Punkten
double dist(vector_t* A, vector_t* B) {
	return hypotf(B->x - A->x, B->y - A->y);
}

// Wo auf der Linie zwischen A und B sitzt der Fußpunkt des Lots von C auf AB?
// Werte kleiner Null bedeuten vor A entlang AB
// Werte zwischen 0 und 1 geben an nach welchem Bruchteil von AB der Fußpunkt kommt
// Werte größer 1 bedeuten daß der Fußpunkt auf der Verlängerung von AB hinter B liegt
// Wert den Code mit A=B aufruf ist doof und verdient daß alles platzt
double dividing_ratio(vector_t* A, vector_t* B, vector_t* C) {
	return ((C->x - A->x)*(B->x - A->x) + (C->y - A->y)*(B->y - A->y))/pow(dist(A, B),2);
}

// Was ist der Minimal-Abstand von C zum Linien AB?
// Achtung dies liefert den vorzeichenbehafteten Abstand
// Werte kleiner Null bedeuten das C "links" der Verbindungslinie liegt wenn man von A Richtung B schaut
// Werte größer Null dementsprechend recht, exakt null auf der Verbindungslinie
double dist_to_line(vector_t* A, vector_t* B, vector_t* C) {
	return ((A->y - B->y)*C->x - (A->x - B->x)* C->y + ((B->y - A->y)* A->x + (A->x - B->x)* A->y))/dist(A,B);
}

// Was ist der Minimal-Abstand von C zum Liniensegment AB?
// Die ist der Abstand zwischen C und dem Fußpunkt des Lots auf AB fall dieser zwischen A und B fällt
// Ansonsten der Abstand zu A btw B
double dist_to_seg(vector_t* A, vector_t* B, vector_t* C) {
	double r = dividing_ratio(A, B, C);
	if(r <= 0) {
		return dist(A, C);
	} else if (r >= 1) {
		return dist(B, C);
	} else {
		return fabs(dist_to_line(A, B, C));
	}
}

waypoint_t* go_around(vector_t* A, vector_t* B, cluster_t* C, double r) {
	vector_t X = { A->x + r*(B->x - A->x), A->y + r*(B->y - A->y) };
	double d = dist(&X, &(C->center));
	vector_t W = {C->center.x + C->safety_radius*sqrt(2)*(X.x - C->center.x) / d, C->center.y + C->safety_radius*sqrt(2)*(X.y - C->center.y) / d};
	waypoint_t* wp = malloc(sizeof(waypoint_t));
	wp->next = NULL;
	wp->point.x = W.x;
	wp->point.y = W.y;
	return wp;
}

waypoint_t* route(vector_t* start, vector_t* stop, cluster_t* cluster, int n_points) {
	int i;

	int i_min = -1;
	double r_min = 1;

	for(i = 0; i < n_points; i++) {
		double r = dividing_ratio(start, stop, &(cluster[i].center));
		if (r > 0 && r < 1) {
			double d = dist_to_line(start, stop, &(cluster[i].center));
			if (fabs(d) < cluster[i].safety_radius) {
				if(fabs(r-0.5) < fabs(r_min-0.5)) {
					i_min = i;
					r_min = r;
				}
			}
		}
	}

	if(i_min >= 0) {
		waypoint_t* wp = go_around(start, stop, &(cluster[i_min]), r_min);

		waypoint_t* part1 = route(start, &(wp->point), cluster, n_points);
		if(part1 == NULL) {
			part1 = wp;
		} else {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = wp;
		}
		waypoint_t* part2 = route(&(wp->point), stop, cluster, n_points);
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

waypoint_t* plotCourse(vector_t* start, vector_t* stop, cluster_t* cluster, int n) {
	int n_points = n*n;
	waypoint_t* wp_start = malloc(sizeof(waypoint_t));
	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	wp_start->point.x = start->x;
	wp_start->point.y = start->y;
	wp_stop->point.x = stop->x;
	wp_stop->point.y = stop->y;
	wp_start->next = route(start, stop, cluster, n_points);
	wp_stop->next = NULL;
	waypoint_t* t = wp_start;

	while (t->next != NULL) {
		t = t->next;
	}

	t->next = wp_stop;
	return wp_start;
}

void get_surrounding_points(vector_t *surrounding_points, cluster_t *clusters, int n, int face_x, int face_y) {
	surrounding_points[0] = clusters[(face_y - 1) * n + (face_x - 1)].center;
	surrounding_points[1] = clusters[(face_y) * n + (face_x - 1)].center;
	surrounding_points[2] = clusters[(face_y) * n + (face_x)].center;
	surrounding_points[3] = clusters[(face_y - 1) * n + (face_x)].center;
}

int get_line_intersection(vector_t *P0, vector_t *P1, vector_t *P2, vector_t *P3, vector_t *result)
{
	float s1_x, s1_y, s2_x, s2_y;
	s1_x = P1->x - P0->x;
	s1_y = P1->y - P0->y;
	s2_x = P3->x - P2->x;
	s2_y = P3->y - P2->y;

	float s, t;
	s = (-s1_y * (P0->x - P2->x) + s1_x * (P0->y - P2->y)) / (-s2_x * s1_y + s1_x * s2_y);
	t = ( s2_x * (P0->y - P2->y) - s2_y * (P0->x - P2->x)) / (-s2_x * s1_y + s1_x * s2_y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		if (result) {
			result->x = P0->x + (t * s1_x);
			result->y = P0->y + (t * s1_y);
		}
		return 1;
	}

	return 0;
}

void find_face(vector_t *p, cluster_t *cluster, int n, int *x, int *y) {
	int face_x = p->x / (GLOBALS.WIDTH / (n + 1));
	int face_y = p->y / (GLOBALS.HEIGHT / (n + 1));
	int edge;
	int retry = 1;
	int count = 0;

	vector_t surrounding[4];

	while (retry && count < 4) {
		retry = 0;
		get_surrounding_points(surrounding, cluster, n, face_x, face_y);

		for (edge = 0; edge < 4; edge++) {
			double r = dist_to_line(surrounding + edge, surrounding + (edge + 1) % 4, p);
			if (r > 0) {

				fprintf(stderr, "edge: %d, r %f, x %d, y %d\n", edge, r, face_x, face_y);
				retry = 1;
				count++;

				switch (edge) {
					case 0 :
						face_x--;
						break;
					case 1 :
						face_y++;
						break;
					case 2 :
						face_x++;
						break;
					case 3 :
						face_y--;
						break;
				}

				break;
			}
		}
	}

	*x = face_x;
	*y = face_y;

}

waypoint_t *smooth(waypoint_t *way, int res) {
	waypoint_t *end = way;
	waypoint_t *working_start;
	waypoint_t *working;

	vector_t startp;
	vector_t midp;
	vector_t endp;
	vector_t t1, t2;
	vector_t v1, v2;
	vector_t s;

	int i;

	working_start = malloc(sizeof(waypoint_t));
	*working_start = *way;
	working = working_start;

	if (end) {
		midp = end->point;
		end  = end->next;
	} else return way;

	if (end) {
		endp = end->point;
		end  = end->next;
	} else return way;


	while (end != NULL) {
		startp = midp;
		midp   = endp;
		endp   = end->point;

		t1.x = (startp.x + midp.x) / 2.;
		t1.y = (startp.y + midp.y) / 2.;

		t2 = midp;

		v1.x = (midp.x - startp.x) / 2. / res;
		v1.y = (midp.y - startp.y) / 2. / res;

		v2.x = (endp.x - midp.x) / 2. / res;
		v2.y = (endp.y - midp.y) / 2. / res;

		for (i = 0; i < res; i++) {
			s.x = t1.x + ((t2.x - t1.x) * i) / res;
			s.y = t1.y + ((t2.y - t1.y) * i) / res;

			t1.x += v1.x;
			t1.y += v1.y;

			t2.x += v2.x;
			t2.y += v2.y;

			working->next = malloc (sizeof(waypoint_t));
			working = working->next;
			working->point = s;
		}
		
		working->next = end;
		end = end->next;
	}

	return working_start;
}

waypoint_t* plotCourse_gridbased(vector_t *start, vector_t *stop, cluster_t *cluster, int n) {
	int face_x, face_y, face_s_x, face_s_y;
	int edge;
	int last_edge = -1;

	find_face(start, cluster, n, &face_x, &face_y);
	find_face(stop, cluster, n, &face_s_x, &face_s_y);
	vector_t surrounding[4];
	waypoint_t *wp_start = malloc (sizeof(waypoint_t));
	wp_start->point = *start;
	wp_start->next  = NULL;

	waypoint_t *working = wp_start;

	while (face_x != face_s_x || face_y != face_s_y) {
		vector_t c;
		get_surrounding_points(surrounding, cluster, n, face_x, face_y);

		for (edge = 0; edge < 4; edge++) {
			if (edge == last_edge) continue;
			c = working->point;

			if (get_line_intersection(surrounding + edge, surrounding + (edge + 1)%4, &c, stop, NULL)) {
				c.x = (surrounding[edge].x + surrounding[(edge + 1) % 4].x) / 2;
				c.y = (surrounding[edge].y + surrounding[(edge + 1) % 4].y) / 2;
				break;
			}
		}

		working->next = malloc(sizeof(waypoint_t));
		working = working->next;
		working->point = c;
		working->next = NULL;

		switch (edge) {
			case 0 :
				face_x--;
				break;
			case 1 :
				face_y++;
				break;
			case 2 :
				face_x++;
				break;
			case 3 :
				face_y--;
				break;
			default:
				return wp_start;
		}

		last_edge = (edge + 2) % 4;
	}

	working->next = malloc(sizeof(waypoint_t));
	working = working->next;
	working->point = *stop;
	working->next = NULL;

	return wp_start;
}
