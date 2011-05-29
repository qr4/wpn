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
#include "config.h"

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


/* List of libraries to be imported into a newly created lua state */
/* Stolen from lua's linit.c */
static const luaL_Reg lualibs[] = {
	{"", luaopen_base},
	//{LUA_LOADLIBNAME, luaopen_package},
	{LUA_TABLIBNAME, luaopen_table},
	//{LUA_IOLIBNAME, luaopen_io},
	//{LUA_OSLIBNAME, luaopen_os},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	//{LUA_DBLIBNAME, luaopen_debug},
	{NULL, NULL}
};


/* Hook to be called if a ship's computer exceeds it's computing time limit,
 * killing the computer. */
static void time_exceeded_hook(lua_State* L, lua_Debug* ar) {

	/* Simply raise an error in the ship's lua state, thus cascading to
	 * kill the computer */
	lua_pushstring(L, "Execution time exceeded.");
	lua_error(L);
}

/* Initialize a new ship's computer.
 *
 * Sets up the lua state, registers all functions available to the ship
 * and runs the player-independent bootcode.
 */
void init_ship_computer(entity_t* s) {

	const luaL_Reg* lib = lualibs;

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

	/* Load libraries we need (code ripped from lua's linit.c) */
	for(; lib->func; lib++) {
		lua_pushcfunction(s->lua, lib->func);
		lua_pushstring(s->lua, lib->name);
		lua_call(s->lua, 1, 0);
	}

	/* registier lua-callable functions */
	lua_register(s->lua, "moveto", lua_moveto);
	lua_register(s->lua, "set_autopilot_to", lua_set_autopilot_to);
	lua_register(s->lua, "killself", lua_killself);
	lua_register(s->lua, "find_closest", lua_find_closest);
	lua_register(s->lua, "entity_to_string", lua_entity_to_string);

	/* Set the execution time limit */
	lua_sethook(s->lua, time_exceeded_hook, LUA_MASKCOUNT, config_get_int("lua_max_cycles"));

	/* Set self-pointer of the ship */
	lua_pushlightuserdata(s->lua, s);
	lua_setglobal(s->lua, "self");


	/* Load the (player-independent) init code into it */
	lua_active_entity = s;
	if(luaL_dofile(s->lua, config_get_string("ship_init_code_file"))) {
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
