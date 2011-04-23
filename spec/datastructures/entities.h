#ifndef ENTITIES_H
#define ENTITIES_H

typedef enum {
	CLUSTER,
	PLANET,
	ASTEROID,
	SHIP,
} type_t;

typedef struct {
	double x;
	double y;
} coordinates_t;

typedef struct {
	union {
		coordinates_t coords;
		struct {
			double x;
			double y;
		};
	};
	type_t type;
	void  *data;
} entity_t;

typedef struct {
	entity_t *entities;
	size_t    nmemb;
	float     radius;
} cluster_data_t;

typedef struct {
	int belongs_to;
} planet_data_t;

typedef struct {
	entity_t *entities;
	size_t n_x;
	size_t n_y;
	double size_x;
	double size_y;
} map_data_t;

// ...

#endif  /*ENTITIES_H*/
