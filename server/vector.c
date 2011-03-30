#include "vector.h"

/*
 * Decide on which side the point C is realtive to the line A - B is.
 *
 * = 0 -> C on the line A - B
 * > 0 -> C left of line A - B
 * < 0 -> C right of line A - B
 */
double relative_position(const vector_t *A, const vector_t *B, const vector_t *C) {
	vector_t t1, t2;
	double t3;
	t1.v = B->v - A->v;
	t2.v = C->v - A->v;

	t3 = t2.x;
	t2.x = t2.y;
	t2.y = t3;

	t1.v *= t2.v;
	return t1.x - t1.y;
}

/*
 * Dividing ratio of C on A - B
 */
double dividing_ratio(vector_t* A, vector_t* B, vector_t* C) {
	vector_t t;
	t.v = (C->v - A->v) * (B->v - A->v);
	return (t.x + t.y) / vector_quaddist(A, B);
}

/*
 * Minimal distance of C to the line A - B
 */
double dist_to_line(vector_t* A, vector_t* B, vector_t* C) {
	return ((A->y - B->y)*C->x - (A->x - B->x)* C->y + ((B->y - A->y)* A->x + (A->x - B->x)* A->y))/vector_dist(A,B);
}
