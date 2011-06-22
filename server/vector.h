#ifndef VECTOR_H
#define VECTOR_H

/*
 * This file includes the vector_t datastructure and provides rudimentary
 * vector functions.
 */

#ifdef __SSE2__
#include <x86intrin.h>
#endif

#include <stdlib.h>
#include <math.h>

typedef double v2d __attribute__ ((vector_size (16)));

typedef union {
	v2d v;
	struct {
		double x;
		double y;
	};
} vector_t;


/*
 * Returns the distance between 2 points squared.
 * This is faster than dist().
 */
static inline double vector_quaddist(const vector_t* A, const vector_t* B) {
	vector_t t;
	t.v = B->v - A->v;
	t.v *= t.v;
	return t.x + t.y;
}

/*
 * Returns the distance between 2 points
 */
static inline double vector_dist(const vector_t* A, const vector_t* B) {
	return sqrt(vector_quaddist(A, B));
}

/*
 * Returns the length of a vector
 */
static inline double vector_length(const vector_t* A) {
	return hypot(A->x, A->y);
}

/*
 * Return a vector with both values set to x
 */
static inline vector_t vector(const double x) {
	return (vector_t) {{x, x}};
}

/*
 * return random vector between {-1, -1} - {1, 1}
 */
static inline vector_t randv() {
	vector_t v;
	v.v = (v2d) { drand48(), drand48() } * vector(2).v - vector(1).v;
	return v;
}

/*
 * Decide on which side the point C is realtive to the line A - B is.
 *
 * = 0 -> C on the line A - B
 * > 0 -> C left of line A - B
 * < 0 -> C right of line A - B
 */
double vector_relative_position(const vector_t *A, const vector_t *B, const vector_t *C);

/*
 * Dividing ratio of C on A - B
 */
double vector_dividing_ratio(vector_t* A, vector_t* B, vector_t* C);

/*
 * Minimal distance of C to the line A - B
 */
double vector_dist_to_line(vector_t* A, vector_t* B, vector_t* C);
#endif  /*VECTOR_H*/
