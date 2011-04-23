#include <stdio.h>
#include "luafuncs.h"
#include "luastate.h"
#include "entities.h"

/* Kill the interpreter, resetting to an empty ship.
 * This just deliberately creates a lua error, thus
 * causing the error handler to kill the ship. */
int lua_killself(lua_State* L) {
	entity_t* e;
	int n;

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
	double x,y;
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
			lua_pop(L,1);

			/* Fall through to the 2-argument case */
		case 2:
			
			/* Get coordinates */
			if(!lua_isnumber(L,-1) || !lua_isnumber(L,-2)) {
				lua_pushstring(L, "coordinates to moveto must me numeric");
				lua_error(L);
			}
			y = lua_tonumber(L,-1);
			x = lua_tonumber(L,-2);

			lua_pop(L,2);
			break;
		default:
			lua_pushstring(L, "Wrong number of arguments to moveto.");
			lua_error(L);
	}

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

	/* Now call the moveto-flightplanner */
	/* TODO: Actually do this. */
	/* moveto_planner(e, x, y, callback) */
	/* Until then: just set the speed to arrive at the target location within 5 timesteps. We do, however, not stop yet. */
	e->v.x = (x - e->pos.x) / 5.;
	e->v.y = (y - e->pos.y) / 5.;

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
			lua_pop(L,1);

			/* Fall through to the 2-argument case */
		case 2:
			
			/* Get coordinates */
			if(!lua_isnumber(L,-1) || !lua_isnumber(L,-2)) {
				lua_pushstring(L, "coordinates to set_autopilot_to must me numeric");
				lua_error(L);
			}
			y = lua_tonumber(L,-1);
			x = lua_tonumber(L,-2);

			lua_pop(L,2);
			break;
		default:
			lua_pushstring(L, "Wrong number of arguments to set_autopilot_to.");
			lua_error(L);
	}

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

	/* Now call the flightplanner */
	/* TODO: Actually do this. */
	/* autopilot_planner(e, x, y, callback) */

	return 0;
}

