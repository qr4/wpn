/*
 * Methods pertaining to handling of a ships lua state (as a whole, in contrast
 * to the lua-callable functions in luafuncs.c)
 */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "luastate.h"
#include "luafuncs.h"
#include "entities.h"

/* Currently active lua entity */
entity_t* lua_active_entity;

/* Function names for event-triggered callbacks */
char* callback_names[NUM_EVENTS] = {
	/* AUTOPILOT_ARRIVED */ "on_autopilot_arrived",
	/* ENTITY_APPROACHING */ "on_entity_approaching",
	/* SHOT_AT */ "on_shot_at",
	/* BEING_DOCKED */ "on_being_docked",
	/* TIMER_EXPIRED */ "on_timer_expired",
};

/* Initialize a new ship's computer.
 *
 * Sets up the lua state, registers all functions available to the ship
 * and runs the player-independent bootcode.
 */
void init_ship_computer(entity_t* s) {
	
	/* If the ship already has a running program, delete it. */
	if(s->lua) {
		lua_close(s->lua);
	}

	/* Initialize a new, blank lua state */
	s->lua = luaL_newstate();
	if(s->lua == NULL) {
		fprintf(stderr, "Creating a new lua state failed for entity %lx\n", (unsigned long) s);
		exit(1);
	}

	/* TODO: Remove these libs, we only want the bare essentials */
	luaL_openlibs(s->lua);

	/* registier lua-callable functions */
	lua_register(s->lua, "moveto", lua_moveto);
	lua_register(s->lua, "set_autopilot_to", lua_set_autopilot_to);
	lua_register(s->lua, "killself", lua_killself);

	/* Set self-pointer of the ship */
	lua_pushlightuserdata(s->lua, s);
	lua_setglobal(s->lua, "self");

	/* Load the (player-independent) init code into it */
	/* TODO: location of this file should be configurable */
	lua_active_entity = s;
	if(luaL_dofile(s->lua, "init.lua")) {
		fprintf(stderr, "Running init-code for entity %lx failed:\n", (unsigned long) s);
		fprintf(stderr, "%s\n", lua_tostring(s->lua,-1));
		kill_computer(s);
	}

	/* It is now ready to go. */
}

/* Kill a ship computer (probably due to an error) */
void kill_computer(entity_t* s) {
	if(s->lua == NULL) {
		/* This ship is already dead. */
		return;
	}

	lua_close(s->lua);
	s->lua = NULL;
}

/* Call an entitie's callback function for a given type of event */
void call_entity_callback(entity_t* e, event_t event) {
	
	char* function_name;

	/* First: check lua-workyness */
	if(!e->lua) {
		return;
	}

	if(event >= NUM_EVENTS) {
		fprintf(stderr, "Attempting to trigger an unknown event: %u\n", event);
		return;
	}

	/* Put the callback function on the top of the stack */
	function_name = callback_names[event];
	lua_getglobal(e->lua, function_name);

	/* Check that it's actually defined */
	if(lua_isnil(e->lua, -1)) {
		lua_pop(e->lua,1);
		return;
	}

	/* Call it.*/
	/* TODO: Pass some relevant arguments to the event? 
	 * alternatively: set information of the craft's environment
	 * as globals in it's lua state */
	if(lua_pcall(e->lua, 0,0,0)) {
		/* Lua errors send the ship adrift */
		kill_computer(e);
	}
}
