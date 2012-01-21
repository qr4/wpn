#ifndef _TYPES_H
#define _TYPES_H
#include <jansson.h>

typedef struct {
	int size;
	int fill;
	char* data;
} buffer_t;

typedef struct {
	json_int_t id;
	float x;
	float y;
	char contents[10];
	char active;
} asteroid_t;

typedef struct {
	json_int_t id;
	float x;
	float y;
	int owner;
	char active;
} planet_t;

typedef struct {
	float x;
	float y;
	float strength;
} explosion_t;

typedef struct {
	json_int_t id;
	float x;
	float y;
	int owner;
	int size;
	char contents[25];
	int docked_to;
	char active;
} ship_t;

typedef struct {
	json_int_t id;
	float x;
	float y;
	int owner;
	int size;
	char contents[25];
	int docked_to;
	char active;
} base_t;

typedef struct {
	float src_x;
	float src_y;
	float trg_x;
	float trg_y;
	float strength;
} shot_t;

typedef struct {
	float xmin;
	float xmax;
	float ymin;
	float ymax;
} bbox_t;

typedef struct {
	int id;
	char* name;
} player_t;

typedef struct options_t options_t;
typedef struct storage_t storage_t;

struct storage_t {
	union {
		asteroid_t*   asteroids;
		base_t*       bases;
		explosion_t*  explosions;
		planet_t*     planets;
		ship_t*       ships;
		shot_t*       shots;
		player_t*     players;
		void*         data;
	};
	unsigned int n;
	unsigned int n_max;
};

struct options_t {
	storage_t asteroids;
	storage_t bases;
	storage_t explosions;
	storage_t planets;
	storage_t ships;
	storage_t shots;
	storage_t players;

	bbox_t boundingbox;
	int local_player ;
};
#endif
