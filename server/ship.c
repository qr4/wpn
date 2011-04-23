#include <lua.h>
#include "ship.h"
#include "vector.h"

/* Create a new, empty ship */
entity_t* create_ship(vector_t pos, uint8_t size) {

	int i;

	entity_t* this = malloc(sizeof(entity_t));
	this->pos = pos;
	this->v.v = (v2d) {0., 0.};
	this->type.ship = 1;
	this->type.slots = size;
	this->type.asteroid=0;
	this->type.planet=0;
	this->type.base=0;
	this->type.cluster=0;

	/* TODO: init content */
	//for(i=0; i<size; i++) {
	//	this->content[i] = ' ';
	//}
	this->lua=NULL;
	init_ship_computer(this);

	return this;
}
