#ifndef _LUAFUNCS_H
#define _LUAFUNCS_H

#include <lua.h>
#include "entities.h"

/* Functions callable from within lua.
 *
 * To make new functions available to the spacecrafts' lua state,
 * also remember to add them in the init_ship_computer function
 * in luastate.c
 */


/* These Functions are grouped by:
 *
 * --- ACTIONS - with which entities interact with the rest of the world --- */
/* Kill the lua state by throwing an error, sends the spacecraft adrift */
extern int lua_killself(lua_State* L);

/* Fly straight to the specified coordinates */
extern int lua_moveto(lua_State* L);

/* Using the (more) intelligent autopilot, fly to the specified coordinates,
 * avoiding planets and asteroid fields */
extern int lua_set_autopilot_to(lua_State* L);

/* Register a timer to re-enter execution after the specified number of ticks */
int lua_set_timer(lua_State* L);

/* Dock with another entity */
int lua_dock(lua_State* L);


/* --- QUERIES - with which entities get information about their surroundings */
/* Search the closest entity in the search radius matchin the filter.
 */
extern int lua_find_closest(lua_State *L);

/* Get the player id of an entity (or yourself) */
int lua_get_player(lua_State* L);

/* Get the position (x- and y-coordinate) of an entity (or yourself) */
int lua_get_position(lua_State* L);

/* --- Debugging and administrative functions --- */
/* Registers all lua_functions available through c-api */
void register_lua_functions(entity_t *s);

/* Debugging helper function, creating a string description of the given entity */
extern int lua_entity_to_string(lua_State* L);

#endif /* _LUAFUNCS_H */