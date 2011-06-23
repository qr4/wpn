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
#include "debug.h"
#include "player.h"
#include "types.h"

/* Currently active lua entity */
entity_id_t lua_active_entity;

/* Function names for event-triggered callbacks */
char* callback_names[NUM_EVENTS] = {
	/* AUTOPILOT_ARRIVED */ "on_autopilot_arrived",
	/* ENTITY_APPROACHING */ "on_entity_approaching",
	/* SHOT_AT */ "on_shot_at",
	/* WEAPONS_READY */ "on_weapons_ready",
	/* BEING_DOCKED */ "on_being_docked",
	/* DOCKING_COMPLETE */ "on_docking_complete",
	/* UNDOCKING_COMPLETE */ "on_undocking_complete",
	/* BEING_UNDOCKED */ "on_being_undocked",
	/* TRANSFER_COMPLETE */ "on_transfer_complete",
	/* BUILD_COMPLETE */ "on_build_complete",
	/* TIMER_EXPIRED */ "on_timer_expired",
	/* MINING_COMPLETE */ "on_mining_complete",
	/* MANUFACTURE_COMPLETE */ "on_manufacture_complete",
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
void time_exceeded_hook(lua_State* L, lua_Debug* ar) {

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
		ERROR("Creating a new lua state failed for entity %lx\n", (unsigned long) s);
		exit(1);
	}

	/* Load libraries we need (code ripped from lua's linit.c) */
	for(; lib->func; lib++) {
		lua_pushcfunction(s->lua, lib->func);
		lua_pushstring(s->lua, lib->name);
		lua_call(s->lua, 1, 0);
	}

	/* registier lua-callable functions */
	register_lua_functions(s);

	/* Set the execution time limit */
	lua_sethook(s->lua, time_exceeded_hook, LUA_MASKCOUNT, config_get_int("lua_max_cycles"));

	/* Set self-pointer of the ship */
	lua_pushlightuserdata(s->lua, (void*)(s->unique_id.id));
	lua_setglobal(s->lua, "self");


	/* Load the (player-independent) init code into it */
	lua_active_entity = s->unique_id;
	if(luaL_dofile(s->lua, config_get_string("ship_init_code_file"))) {
		ERROR("Running init-code for entity %lx failed:\n", (unsigned long) s);
		ERROR("%s\n", lua_tostring(s->lua,-1));
		kill_computer(s);
	}

	/* It is now ready to go. */
}

/* Kill a ship computer (probably due to an error) */
void kill_computer(entity_t* s) {
	player_data_t* p;

	if(s->lua == NULL) {
		/* This ship is already dead. */
		return;
	}

	/* Do not kill the lua state of a homebase */
	p = find_player(s->player_id);
	if(p && p->homebase.id == s->unique_id.id) {
		return;
	}

	lua_close(s->lua);
	s->lua = NULL;
}

/* Call an entitie's callback function for a given type of event */
void call_entity_callback(entity_t* e, event_t event, entity_id_t context) {

	char* function_name;
	entity_id_t prev_active_entity;

	/* First: check lua-workyness */
	if(!e->lua) {
		return;
	}

	if(event >= NUM_EVENTS) {
		ERROR("Attempting to trigger an unknown event: %u\n", event);
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

	/* Push the "context" entity on the stack as well. If it is invalid, push self. */
	if(context.id != INVALID_ID.id) {
		lua_pushlightuserdata(e->lua, (void*)(context.id));
	} else {
		lua_pushlightuserdata(e->lua,(void*)(e->unique_id.id));
	}

	/* Make this entity "active" */
	prev_active_entity = lua_active_entity;
	lua_active_entity = e->unique_id;

	/* Set the execution time limit */
	lua_sethook(e->lua, time_exceeded_hook, LUA_MASKCOUNT, config_get_int("lua_max_cycles"));

	/* Call it.*/
	if(lua_pcall(e->lua, 1,0,0)) {

		/* Lua errors send the ship adrift */
		DEBUG("Entity %u killed due to lua error:\n%s\n", e->unique_id.id, lua_tostring(e->lua, -1));
		kill_computer(e);
	}

	/* Whatever was active previously is now active again */
	lua_active_entity = prev_active_entity;
}
