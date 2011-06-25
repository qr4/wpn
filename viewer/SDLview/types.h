#ifndef _TYPES_H
#define _TYPES_H

typedef struct {
	int size;
	int fill;
	char* data;
} buffer_t;

typedef struct {
	int id;
	float x;
	float y;
	char contents[10];
	char active;
} asteroid_t;

typedef struct {
	int id;
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
	int id;
	float x;
	float y;
	int owner;
	int size;
	char contents[10];
	int docked_to;
	char active;
} ship_t;

typedef struct {
	int id;
	float x;
	float y;
	int owner;
	int size;
	char contents[10];
	int docked_to;
	char active;
} base_t;

typedef struct {
	float src_x;
	float src_y;
	float trg_x;
	float trg_y;
} shot_t;

typedef struct {
	float xmin;
	float xmax;
	float ymin;
	float ymax;
} bbox_t;

#endif
