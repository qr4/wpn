#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "route.h"
#include "globals.h"

// Quadrierter Abstand
// Mit Vorzeichen
double quaddist(vector_t* A, vector_t* B) {
	vector_t t;
	t.v = B->v - A->v;
	t.v *= t.v;
	return t.x + t.y;
}

// Abstand zwischen zwei Punkten
double dist(vector_t* A, vector_t* B) {
	return sqrt(quaddist(A, B));
}

double left_of(const vector_t *A, const vector_t *B, const vector_t *C) {
	vector_t t1, t2;
	double t3;
	t1.v = B->v - A->v;
	t2.v = C->v - A->v;

	t3 = t2.x;
	t2.x = t2.y;
	t2.y = t3;

	t1.v *= t2.v;
	t3 = t1.x - t1.y;
	return t3;
}

// Wo auf der Linie zwischen A und B sitzt der Fußpunkt des Lots von C auf AB?
// Werte kleiner Null bedeuten vor A entlang AB
// Werte zwischen 0 und 1 geben an nach welchem Bruchteil von AB der Fußpunkt kommt
// Werte größer 1 bedeuten daß der Fußpunkt auf der Verlängerung von AB hinter B liegt
// Wert den Code mit A=B aufruf ist doof und verdient daß alles platzt
double dividing_ratio(vector_t* A, vector_t* B, vector_t* C) {
	return ((C->x - A->x)*(B->x - A->x) + (C->y - A->y)*(B->y - A->y))/quaddist(A, B);
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
	vector_t X;
	X.v = A->v + (v2d) {r, r} * (B->v - A->v);;
	double d = dist(&X, &(C->center));
	vector_t W;// = {{C->center.x + C->safety_radius*sqrt(2)*(X.x - C->center.x) / d, C->center.y + C->safety_radius*sqrt(2)*(X.y - C->center.y) / d}};
	W.v = C->center.v + (X.v - C->center.v) * (v2d) {M_SQRT2, M_SQRT2} * (v2d) {C->safety_radius,  C->safety_radius} / (v2d) {d, d};
	waypoint_t* wp = malloc(sizeof(waypoint_t));
	wp->next = NULL;
	wp->point = W;
	return wp;
}

void get_surrounding_points(cluster_t **surrounding_points, cluster_t* clusters, int n, int face_x, int face_y) {
	surrounding_points[0] = &(clusters[(face_y - 1) * n + (face_x - 1)]);
	surrounding_points[1] = &(clusters[(face_y) * n + (face_x - 1)]);
	surrounding_points[2] = &(clusters[(face_y) * n + (face_x)]);
	surrounding_points[3] = &(clusters[(face_y - 1) * n + (face_x)]);
}

int get_line_intersection(vector_t *P0, vector_t *P1, vector_t *P2, vector_t *P3, vector_t *result)
{
	vector_t s1, s2;
	s1.v = P1->v - P0->v;
	s2.v = P3->v - P2->v;

	double s, t;
	t = s1.x * s2.y - s2.x * s1.y;
	s = (-s1.y * (P0->x - P2->x) + s1.x * (P0->y - P2->y)) / t;
	t = ( s2.x * (P0->y - P2->y) - s2.y * (P0->x - P2->x)) / t;

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		if (result) {
			result->v = P0->v + ((v2d) {t, t} * s1.v);
		}
		return 1;
	}

	return 0;
}

void find_face(vector_t *p, cluster_t *clusters, int n, int *x, int *y) {
	int face_x = p->x / (GLOBALS.WIDTH / (n + 1));
	int face_y = p->y / (GLOBALS.HEIGHT / (n + 1));
	int edge;
	int retry = 1;
	int count = 0;

	cluster_t *surrounding[4];

	while (retry && count < 4) {
		retry = 0;
		get_surrounding_points(surrounding, clusters, n, face_x, face_y);

		for (edge = 0; edge < 4; edge++) {
			double r = left_of(&(surrounding[edge]->center), &(surrounding[(edge + 1) % 4]->center), p);
			if (r > 0) {

				//fprintf(stderr, "edge: %d, r %f, x %d, y %d\n", edge, r, face_x, face_y);
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

waypoint_t* route_scanline_gridbased(vector_t* start, vector_t* stop, 
		int face_start_x, int face_start_y, int face_stop_x, int face_stop_y, 
		cluster_t* clusters, int n) {
	static int called = 0;
	int i = -1;
	int last_edge = -1;
	int face_x = face_start_x;
	int face_y = face_start_y;

	cluster_t *min = NULL;
	double r_min = 1;

	called++;

	//fprintf(stderr, "going from (%d, %d) to (%d, %d)\n", face_start_x, face_start_y, face_stop_x, face_stop_y);
	//fprintf(stderr, "going from (%.1f, %.1f) to (%.1f, %.1f)\n", start->x, start->y, stop->x, stop->y);
	while (min == NULL && (face_x != face_stop_x || face_y != face_stop_y)) {
		//printf("x %d y %d sx %d sy %d called: %d i: %d\n", face_x, face_y, face_stop_x, face_stop_y, called, i);
		//getchar();
		cluster_t *surrounding[4];
		get_surrounding_points(surrounding, clusters, n, face_x, face_y);

		for (i = 0; i < 4; i++) {
			double r = dividing_ratio(start, stop, &(surrounding[i]->center));
			if (r > 0 && r < 1) {
				double d = dist_to_line(start, stop, &(surrounding[i]->center));
				if (fabs(d) < surrounding[i]->safety_radius) {
					min = surrounding[i];
					r_min = r;
					break;
				}
			}
		}

		if (min == NULL) {
			for (i = 0; i < 4; i++) {
				if (i == last_edge) {
					continue;
				}

				if (get_line_intersection(&(surrounding[i]->center), &(surrounding[(i + 1)%4]->center), start, stop, NULL)) {
					break;
				}
			}

			switch (i) {
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
					break;
			}

			last_edge = (i + 2) % 4;
		}
	}

	if(min != NULL) {
		waypoint_t* wp = go_around(start, stop, min, r_min);
		find_face(&(wp->point), clusters, n, &face_x, &face_y);

		waypoint_t* part1 = NULL;
		if (face_start_x != face_x || face_start_y != face_y ) {
			part1 = route_scanline_gridbased(start, &(wp->point), face_start_x, face_start_y, face_x, face_y, clusters, n);
		}
		if(part1 == NULL) {
			part1 = wp;
		} else {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = wp;
		}
		waypoint_t* part2 = NULL;
		if (face_stop_x != face_x || face_stop_y != face_y ) {
			 part2 = route_scanline_gridbased(&(wp->point), stop, face_x, face_y, face_stop_x, face_stop_y, clusters, n);
		}
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

waypoint_t* route(vector_t* start, vector_t* stop, cluster_t* clusters, int n_points) {
	int i;

	int i_min = -1;
	double r_min = 1;

	for(i = 0; i < n_points; i++) {
		double r = dividing_ratio(start, stop, &(clusters[i].center));
		if (r > 0 && r < 1) {
			double d = dist_to_line(start, stop, &(clusters[i].center));
			if (fabs(d) < clusters[i].safety_radius) {
				if(fabs(r-0.5) < fabs(r_min-0.5)) {
					i_min = i;
					r_min = r;
				}
			}
		}
	}

	if(i_min >= 0) {
		waypoint_t* wp = go_around(start, stop, &(clusters[i_min]), r_min);

		waypoint_t* part1 = route(start, &(wp->point), clusters, n_points);
		if(part1 == NULL) {
			part1 = wp;
		} else {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = wp;
		}
		waypoint_t* part2 = route(&(wp->point), stop, clusters, n_points);
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

waypoint_t* plotCourse_scanline_gridbased(vector_t* start, vector_t* stop, cluster_t* clusters, int n) {
	//int n_points = n*n;
	int face_start_x, face_start_y, face_stop_x, face_stop_y;
	find_face(start, clusters, n, &face_start_x, &face_start_y);
	find_face(stop, clusters, n, &face_stop_x, &face_stop_y);
	waypoint_t* wp_start = malloc(sizeof(waypoint_t));
	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	wp_start->point = *start;
	wp_stop->point = *stop;
	wp_start->next = route_scanline_gridbased(start, stop, face_start_x, face_start_y, face_stop_x, face_stop_y, clusters, n);
	wp_stop->next = NULL;
	waypoint_t* t = wp_start;

	while (t->next != NULL) {
		t = t->next;
	}

	t->next = wp_stop;
	return wp_start;
}

waypoint_t* plotCourse(vector_t* start, vector_t* stop, cluster_t* clusters, int n) {
	waypoint_t* wp_start = malloc(sizeof(waypoint_t));
	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	wp_start->point = *start;
	wp_stop->point = *stop;
	wp_start->next = route(start, stop, clusters, n * n);
	wp_stop->next = NULL;
	waypoint_t* t = wp_start;

	while (t->next != NULL) {
		t = t->next;
	}

	t->next = wp_stop;
	return wp_start;
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

		t1.v = (startp.v + midp.v) * (v2d) {0.5, 0.5};
		t2 = midp;
		v1.v = (midp.v - startp.v) * (v2d) {0.5, 0.5} / (v2d) {res, res};
		v2.v = (endp.v - midp.v) * (v2d) {0.5, 0.5} / (v2d) {res, res};

		for (i = 0; i < res; i++) {
			s.v = t1.v + ((t2.v - t1.v) * (v2d) {i, i}) / (v2d) {res, res};

			t1.v += v1.v;
			t2.v += v2.v;

			working->next = malloc (sizeof(waypoint_t));
			working = working->next;
			working->point = s;
		}

		working->next = end;
		end = end->next;
	}

	return working_start;
}

waypoint_t* plotCourse_gridbased(vector_t *start, vector_t *stop, cluster_t *clusters, int n) {
	int face_x, face_y, face_s_x, face_s_y;
	int edge;
	int last_edge = -1;

	find_face(start, clusters, n, &face_x, &face_y);
	find_face(stop, clusters, n, &face_s_x, &face_s_y);
	cluster_t *surrounding[4];
	waypoint_t *wp_start = malloc (sizeof(waypoint_t));
	wp_start->point = *start;
	wp_start->next  = NULL;

	waypoint_t *working = wp_start;

	while (face_x != face_s_x || face_y != face_s_y) {
		vector_t c;
		get_surrounding_points(surrounding, clusters, n, face_x, face_y);

		for (edge = 0; edge < 4; edge++) {
			if (edge == last_edge) continue;
			c = working->point;

			if (get_line_intersection(&(surrounding[edge]->center), &(surrounding[(edge + 1)%4]->center), &c, stop, NULL)) {
				c.v = (surrounding[edge]->center.v + surrounding[(edge + 1) % 4]->center.v) / (v2d){2, 2};
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
