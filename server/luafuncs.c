#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "luafuncs.h"
#include "luastate.h"
#include "entities.h"

entity_t *get_self(lua_State *L) {
	entity_t *e;

	/* Fetch self-pointer */
	lua_getglobal(L, "self");
	if(!lua_islightuserdata(L,-1)) {
		lua_pop(L,1);
		lua_pushstring(L, "Your self-pointer is no longer an entity pointer! What have you done?");
		lua_error(L);
	}

	/* Confirm that it is pointing at the active entity */
	e = lua_touserdata(L, -1);
	lua_pop(L,1);
	if(e != lua_active_entity) {
		lua_pushstring(L,"Your self-pointer is not pointing at yourself! What are you doing?");
		lua_error(L);
	}

	return e;
}

/* Kill the interpreter, resetting to an empty ship.
 * This just deliberately creates a lua error, thus
 * causing the error handler to kill the ship. */
int lua_killself(lua_State* L) {
	//entity_t* e = get_self(L);
	//int n;

	/* Invoke a lua error. */
	lua_pushstring(L,"invoked killself()");
	lua_error(L);
	return 0;
}

/* Fly straight to the specified coordinates, ignoring any object in the way
 * Arguments: x, y[, on_arrival]
 * where:
 * 				x: x-coordinate of target location
 * 				y: y-coordinate of target location
 * 				on_arrival: name of the callback to invoke on arrival (optional)
 * 	Return value: none.
 *
 * 	TODO: pass the callback as a string, or as an anonymous function?
 */
int lua_moveto(lua_State* L) {
	entity_t* e;
	int n;
	double x = 0;
	double y = 0;
	char* callback=NULL;

	/* Check number of arguments */
	n = lua_gettop(L);
	switch(n) {
		case 3:
			/* Get callback name */
			if(!lua_isstring(L,-1)) {
				lua_pushstring(L, "Third argument to moveto must be a function name");
				lua_error(L);
			}
			callback = strdup(lua_tostring(L,-1));

			/* Pop it from the stack */
			lua_pop(L,2);

			/* Fall through to the 2-argument case */
		case 2:

			/* Get coordinates */
			if(!lua_isnumber(L,-1) || !lua_isnumber(L,-2)) {
				lua_pushstring(L, "coordinates to moveto must me numeric");
				lua_error(L);
			}
			y = lua_tonumber(L,-1);
			x = lua_tonumber(L,-2);

			lua_pop(L,3);
			break;
		default:
			lua_pushstring(L, "Wrong number of arguments to moveto.");
			lua_error(L);
	}

	e = get_self(L);

	/* Now call the moveto-flightplanner */
	/* TODO: Actually do this. */
	/* moveto_planner(e, x, y, callback) */
	/* Until then: just set the speed to arrive at the target location within 5 timesteps. We do, however, not stop yet. */

	e->v.v = (((v2d) {x, y}) - e->pos.v) / vector(5).v;

	return 0;
}

/* Fly to the specified coordinates, using the intelligent autopilot
 * Arguments: x, y[, on_arrival]
 * where:
 * 				x: x-coordinate of target location
 * 				y: y-coordinate of target location
 * 				on_arrival: name of the callback to invoke on arrival (optional)
 * 	Return value: none.
 *
 * 	TODO: pass the callback as a string, or as an anonymous function?
 */
int lua_set_autopilot_to(lua_State* L) {
	entity_t* e;
	int n;
	double x,y;
	char* callback=NULL;

	/* Check number of arguments */
	n = lua_gettop(L);
	switch(n) {
		case 3:
			/* Get callback name */
			if(!lua_isstring(L,-1)) {
				lua_pushstring(L, "Third argument to set_autopilot_to must be a function name");
				lua_error(L);
			}
			callback = strdup(lua_tostring(L,-1));

			/* Pop it from the stack */
			lua_pop(L,2);

			/* Fall through to the 2-argument case */
		case 2:

			/* Get coordinates */
			if(!lua_isnumber(L,-1) || !lua_isnumber(L,-2)) {
				lua_pushstring(L, "coordinates to set_autopilot_to must me numeric");
				lua_error(L);
			}
			y = lua_tonumber(L,-1);
			x = lua_tonumber(L,-2);

			lua_pop(L,3);
			break;
		default:
			lua_pushstring(L, "Wrong number of arguments to set_autopilot_to.");
			lua_error(L);
	}

	e = get_self(L);

	/* Now call the flightplanner */
	/* TODO: Actually do this. */
	/* autopilot_planner(e, x, y, callback) */

	return 0;
}

/*
 * Returns the nearest object within the radius if filter matches. 
 * Returns NULL if no object is found.
 */
int lua_find_closest(lua_State *L) {
	entity_t *e;
	int n;
	double search_radius;
	unsigned int filter;

	n = lua_gettop(L);

	if (n != 2) {
		lua_pushstring(L, "Invalid number of arguments for find_closest()!");
		lua_error(L);
	}

	filter        = (unsigned int) lua_tonumber(L, -1);
	search_radius = (double) lua_tonumber(L, -2);
	lua_pop(L, 3);
	e = get_self(L);

	lua_pushlightuserdata(L, find_closest(e, search_radius, filter));

	return 1;
}

/* Debugging helper function, creating a string description of the given entity */
int lua_entity_to_string(lua_State* L) {
	entity_t* e;
	char* s;
	char* temp;
	int n;

	/* Check arguments */
	n = lua_gettop(L);
	if(n != 1 || !lua_islightuserdata(L,-1)) {
		lua_pushstring(L, "entity_to_string expects exactly one entity pointer\n");
		lua_error(L);
	}

	/* Pop entity pointer from stack */
	e = lua_touserdata(L, -1);
	lua_pop(L,2);

	if (e == NULL) {
		lua_pushstring(L, "nil");
	} else {
		/* Fill in description */
		temp = slots_to_string(e);
		if(asprintf(&s, "{\n\tEntity %lx:\n\tpos: %f,  %f\n\tv: %f, %f\n\ttype: %s\n\tslots: %i\n\tplayer: %i\n\tcontents: [%s]\n}\n",
					(size_t)e, e->pos.x, e->pos.y, e->v.x, e->v.y, type_string(e->type), e->slots, e->player_id, temp));

		/* Return it */
		lua_pushstring(L,s);
		free(temp);
		free(s);
	}
	return 1;
}
