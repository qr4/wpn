#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "luafuncs.h"
#include "luastate.h"
#include "entities.h"
#include "storages.h"
#include "debug.h"

/* Type of functions which we make available to lua states */
typedef struct {
	lua_CFunction c_function;
	const char lua_function_name[32];
} lua_function_entry;

/* Add all functions that shall be available for the shipcomputers here */
static const lua_function_entry lua_wrappers[] = {
//   C-function name,      Lua-function name
	{lua_moveto,           "moveto"},
	{lua_set_autopilot_to, "set_autopilot_to"},
	{lua_killself,         "killself"},
	{lua_find_closest,     "find_closest"},       
	{lua_entity_to_string, "entity_to_string"},
	{lua_set_timer,        "set_timer"},
	{lua_get_player,       "get_player"}
};

void register_lua_functions(entity_t *s) {
	size_t i;
	for (i = 0; i < sizeof(lua_wrappers) / sizeof(lua_function_entry); i++) {
		DEBUG("Registering \"%s\" at 0x%lx for 0x%lx\n", lua_wrappers[i].lua_function_name, lua_wrappers[i].c_function, s);
		lua_register(s->lua, lua_wrappers[i].lua_function_name, lua_wrappers[i].c_function);
	}
}

static entity_id_t get_self(lua_State *L) {
	entity_id_t e;

	/* Fetch self-pointer */
	lua_getglobal(L, "self");
	if(!lua_islightuserdata(L,-1)) {
		lua_pop(L,1);
		lua_pushstring(L, "Your self-pointer is no longer an entity pointer! What have you done?");
		lua_error(L);
	}

	/* Confirm that it is pointing at the active entity */
	e.id = (uint64_t) lua_touserdata(L, -1);
	lua_pop(L,1);
	if(e.id != lua_active_entity.id) {
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

/* Stop the ship as soon as possible. If we lost all thrust that might mean lithobreaking
 * Arguments: [on_arrival]
 * where:
 * 				on_arrival: name of the callback to invoke on arrival (optional)
 * 	Return value: none.
 *
 * 	TODO: pass the callback as a string, or as an anonymous function?
 */
int lua_stop(lua_State* L) {
	entity_t* e;
	entity_id_t id;
	int n;
	char* callback=NULL;

	/* Check number of arguments */
	n = lua_gettop(L);
	switch(n) {
		case 1:
			/* Get callback name */
			if(!lua_isstring(L,-1)) {
				lua_pushstring(L, "argument to stop must be a function name");
				lua_error(L);
			}
			callback = strdup(lua_tostring(L,-1));

			/* Pop it from the stack */
			lua_pop(L,0);

			/* Fall through to the 0-argument case */
		case 0:
			break;
		default:
			lua_pushstring(L, "Wrong number of arguments to stop.");
			lua_error(L);
	}

	id = get_self(L);
	e = get_entity_by_id(id);

	/* Now call the moveto-flightplanner */
	/* TODO: Actually do this. */
	/* stop_planner(e, x, y, callback) */
	/* Until then: just set the speed to zero */

	e->v.v = (v2d) {0, 0};

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
	entity_id_t id;
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

	id = get_self(L);
	e = get_entity_by_id(id);

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
	entity_id_t id;
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

	id = get_self(L);
	e = get_entity_by_id(id);

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
	entity_id_t id;
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
	id = get_self(L);
	e = get_entity_by_id(id);

	lua_pushlightuserdata(L, (void*)(find_closest(e, search_radius, filter)->unique_id.id));

	return 1;
}

/* Debugging helper function, creating a string description of the given entity */
int lua_entity_to_string(lua_State* L) {
	entity_t* e;
	entity_id_t id;
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
	id.id = (uint64_t) lua_touserdata(L, -1);
	lua_pop(L,2);
	e = get_entity_by_id(id);

	if (e == NULL) {
		lua_pushstring(L, "nil");
	} else {
		/* Fill in description */
		temp = slots_to_string(e);
		if(asprintf(&s, "{\n\tEntity %li:\n\tpos: %f,  %f\n\tv: %f, %f\n\ttype: %s\n\tslots: %i\n\tplayer: %i\n\tcontents: [%s]\n}\n",
					(size_t)e, e->pos.x, e->pos.y, e->v.x, e->v.y, type_string(e->type), e->slots, e->player_id, temp));

		/* Return it */
		lua_pushstring(L,s);
		free(temp);
		free(s);
	}
	return 1;
}


/* Register a timer to re-enter execution after the specified number of ticks */
int lua_set_timer(lua_State* L) {
	entity_id_t id;
	entity_t* e;
	int n;

	n = lua_gettop(L);
	if(n!=1 || !lua_isnumber(L,-1)) {
		lua_pushstring(L, "set_timer expects exactly one integer (number of ticks to wait");
		lua_error(L);
	}

	/* Get number of ticks to wait */
	n = lua_tonumber(L,-1);
	lua_pop(L,1);

	/* Negative timer-values mean: don't wait at all */
	if(n < 0) {
		n = 0;
	}

	/* Get entity-pointer */
	id=get_self(L);
	e = get_entity_by_id(id);

	if(!e) {
		/* Err... what? How can this entity be a null-pointer? */
		ERROR("Entity with lua-state %lx resolves to a NULL-Pointer.\n", (uint64_t)L);
		return 0;
	}

	if(e->ship_data->timer_value != -1) {
		/* Another timer is already ticking for this entity, so we don't set one.
		 * Return 0 to inform the ship.*/
		lua_pushnumber(L,0);
		return 1;
	}

	/* Setup the actual timer */
	e->ship_data->timer_value = n;
	e->ship_data->timer_event = TIMER_EXPIRED;

	/* Return 1 */
	lua_pushnumber(L,1);
	return 1;
}

/* Get the player id of an entity (or yourself) */
int lua_get_player(lua_State* L) {
	entity_id_t id;
	entity_t* e;
	int n;

	n = lua_gettop(L);
	switch(n) {
		case 0: /* No argument -> get own player ID */
			id = get_self(L);
			e = get_entity_by_id(id);
			lua_pushnumber(L,e->player_id);
			return 1;
		case 1: /* One argument -> get ID of target */
			if(!lua_islightuserdata(L,-1)) {
				lua_pushstring(L, "Argument to get_player was not an entity pointer!");
				lua_error(L);
			}

			id.id = (uint64_t) lua_touserdata(L,-1);
			lua_pop(L,1);

			e = get_entity_by_id(id);
			if(e == NULL) {
				/* Return nil if the entity doesn't exist */
				return 0;
			} else {
				lua_pushnumber(L,e->player_id);
				return 1;
			}
		default :
			lua_pushstring(L, "get_player expects exactly one entity argument");
			lua_error(L);
			return 0;
	}
}
