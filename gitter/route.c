#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "route.h"
#include "globals.h"

// Abstand zwischen zwei Punkten
double dist(pixel_t* A, pixel_t* B) {
	return hypotf(B->x - A->x, B->y - A->y);
}

// Wo auf der Linie zwischen A und B sitzt der Fußpunkt des Lots von C auf AB?
// Werte kleiner Null bedeuten vor A entlang AB
// Werte zwischen 0 und 1 geben an nach welchem Bruchteil von AB der Fußpunkt kommt
// Werte größer 1 bedeuten daß der Fußpunkt auf der Verlängerung von AB hinter B liegt
// Wert den Code mit A=B aufruf ist doof und verdient daß alles platzt
double dividing_ratio(pixel_t* A, pixel_t* B, pixel_t* C) {
	return ((C->x - A->x)*(B->x - A->x) + (C->y - A->y)*(B->y - A->y))/pow(dist(A, B),2);
}

// Was ist der Minimal-Abstand von C zum Linien AB?
// Achtung dies liefert den vorzeichenbehafteten Abstand
// Werte kleiner Null bedeuten das C "links" der Verbindungslinie liegt wenn man von A Richtung B schaut
// Werte größer Null dementsprechend recht, exakt null auf der Verbindungslinie
double dist_to_line(pixel_t* A, pixel_t* B, pixel_t* C) {
	//return ((A->y - B->y)*C->x + (A->x - B->x)*C->y + (A->x * B->y - A->y * B->x))/dist(A,B);
	//return ((B->y - A->y)*C->x + (A->x - B->x)* C->y - ((B->y - A->y)* A->x + (A->x - B->x)* A->y))/dist(A,B);
	return ((A->y - B->y)*C->x - (A->x - B->x)* C->y + ((B->y - A->y)* A->x + (A->x - B->x)* A->y))/dist(A,B);
}

// Was ist der Minimal-Abstand von C zum Liniensegment AB?
// Die ist der Abstand zwischen C und dem Fußpunkt des Lots auf AB fall dieser zwischen A und B fällt
// Ansonsten der Abstand zu A btw B
double dist_to_seg(pixel_t* A, pixel_t* B, pixel_t* C) {
	double r = dividing_ratio(A, B, C);
	if(r <= 0) {
		return dist(A, C);
	} else if (r >= 1) {
		return dist(B, C);
	} else {
		return fabs(dist_to_line(A, B, C));
	}
}

waypoint_t* go_around(pixel_t* A, pixel_t* B, pixel_t* C, double r) {
	pixel_t X = { A->x + r*(B->x - A->x), A->y + r*(B->y - A->y) };
	double d = dist(&X, C);
	pixel_t W = {C->x + safety_radius*sqrt(2)*(X.x - C->x) / d, C->y + safety_radius*sqrt(2)*(X.y - C->y) / d};
	waypoint_t* wp = malloc(sizeof(waypoint_t));
	wp->next = NULL;
	wp->point.x = W.x;
	wp->point.y = W.y;
	//printf("Suggest you go via (%f,%f) to avoid (%f,%f)\n", W.x, W.y, C->x, C->y);
	return wp;
}

waypoint_t* route(pixel_t* start, pixel_t* stop, pixel_t* points, int n_points) {
	//printf("Running route\n");

	/*
	pixel_t A = {0, 0};
	pixel_t B = {1000, 0};
	pixel_t C1 = {500, 200};
	printf("A = (0,0), B=(1000,0), C1=(500,200)\n");
	printf("dist(A,B) = %f\n", dist(&A, &B));
	printf("r(AB, C1) = %f\n", dividing_ratio(&A, &B, &C1));
	printf("dist_to_seg(A, B, C1) = %f\n", dist_to_seg(&A, &B, &C1));
	pixel_t C2 = {-100, 200};
	printf("C2 = (-100,200)\n");
	printf("r(AB, C2) = %f\n", dividing_ratio(&A, &B, &C2));
	printf("dist_to_seg(A, B, C2) = %f\n", dist_to_seg(&A, &B, &C2));
	pixel_t C3 = {200, -400};
	printf("C3 = (200,-400)\n");
	printf("r(AB, C3) = %f\n", dividing_ratio(&A, &B, &C2));
	printf("dist_to_seg(A, B, C3) = %f\n", dist_to_seg(&A, &B, &C3));
	*/

	int i;

	int i_min = -1;
	double r_min = 1;

	for(i = 0; i < n_points; i++) {
		if((points[i].x < start->x - safety_radius) && (points[i].x < stop->x - safety_radius)) {
			continue;
		}
		if((points[i].x > start->x + safety_radius) && (points[i].x > stop->x + safety_radius)) {
			continue;
		}
		if((points[i].y < start->y - safety_radius) && (points[i].y < stop->y - safety_radius)) {
			continue;
		}
		if((points[i].y > start->y + safety_radius) && (points[i].y > stop->y + safety_radius)) {
			continue;
		}
		double r = dividing_ratio(start, stop, &(points[i]));
		if (r > 0 && r < 1) {
			double d = dist_to_line(start, stop, &(points[i]));
			if (fabs(d) < safety_radius) {
				//printf("Point #%d at (%f,%f) is an obstacle %f pixel away from the course\n", i, points[i].x, points[i].y, d);
				if(r < r_min) {
					i_min = i;
					r_min = r;
				}
			}
		}
	}

	if(i_min >= 0) {
		printf("Point #%d at (%f,%f) is the first obstacle %f pixel down the course\n", i_min, points[i_min].x, points[i_min].y, r_min * dist(start, stop));
		waypoint_t* wp = go_around(start, stop, &(points[i_min]), r_min);

		waypoint_t* part1 = route(start, &(wp->point), points, n_points);
		if(part1 == NULL) {
			part1 = wp;
		} else {
			waypoint_t* t = part1;
			while(t->next != NULL) {
				t = t->next;
			}
			t->next = wp;
		}
		waypoint_t* part2 = route(&(wp->point), stop, points, n_points);
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

waypoint_t* plotCourse(pixel_t* start, pixel_t* stop, pixel_t* points, int n) {
	int n_points = n*n;
	waypoint_t* wp_start = malloc(sizeof(waypoint_t));
	waypoint_t* wp_stop = malloc(sizeof(waypoint_t));
	wp_start->point.x = start->x;
	wp_start->point.y = start->y;
	wp_stop->point.x = stop->x;
	wp_stop->point.y = stop->y;
	wp_start->next = route(start, stop, points, n_points);
	wp_stop->next = NULL;
	waypoint_t* t = wp_start;

	while (t->next != NULL) {
		t = t->next;
	}

	t->next = wp_stop;
	return wp_start;
}

void get_surrounding_points(pixel_t *surrounding_points, pixel_t *points, int n, int face_x, int face_y) {
	surrounding_points[0] = points[(face_y - 1) * n + (face_x - 1)];
	surrounding_points[1] = points[(face_y) * n + (face_x - 1)];
	surrounding_points[2] = points[(face_y) * n + (face_x)];
	surrounding_points[3] = points[(face_y - 1) * n + (face_x)];
}

int get_line_intersection(pixel_t *P0, pixel_t *P1, pixel_t *P2, pixel_t *P3, pixel_t *result)
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

void find_face(pixel_t *p, pixel_t *points, int n, int *x, int *y) {
	int face_x = p->x / (GLOBALS.WIDTH / (n + 1));
	int face_y = p->y / (GLOBALS.HEIGHT / (n + 1));
	int edge;
	int retry = 1;
	int count = 0;

	pixel_t surrounding[4];

	while (retry && count < 4) {
		retry = 0;
		get_surrounding_points(surrounding, points, n, face_x, face_y);

		for (edge = 0; edge < 4; edge++) {
			double r = dist_to_line(surrounding + edge, surrounding + (edge + 1) % 4, p);
			if (r > 0) {

				printf("edge: %d, r %f, x %d, y %d\n", edge, r, face_x, face_y);
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

	pixel_t startp;
	pixel_t midp;
	pixel_t endp;
	pixel_t t1, t2;
	pixel_t v1, v2;
	pixel_t s;

	int i;

	working_start = malloc(sizeof(waypoint_t));
	*working_start = *way;
	working_start->next = NULL;
	working = working_start;

	if (end) {
		midp = end->point;
		end  = end->next;
	} else return NULL;

	if (end) {
		endp = end->point;
		end  = end->next;
	} else return NULL;


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

waypoint_t* plotCourse_gridbased(pixel_t *start, pixel_t *stop, pixel_t *points, int n) {
	int face_x, face_y, face_s_x, face_s_y;
	int edge;
	int last_edge = -1;

	find_face(start, points, n, &face_x, &face_y);
	find_face(stop, points, n, &face_s_x, &face_s_y);
	pixel_t surrounding[4];
	waypoint_t *wp_start = malloc (sizeof(waypoint_t));
	wp_start->point = *start;
	wp_start->next  = NULL;

	waypoint_t *working = wp_start;

	while (face_x != face_s_x || face_y != face_s_y) {
		pixel_t c;
		get_surrounding_points(surrounding, points, n, face_x, face_y);

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
