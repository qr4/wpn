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

/* Undock again */
int lua_undock(lua_State* L);

/* Transfer (swap) slots to/from the docking partner */
int lua_transfer_slot(lua_State* L);

/* Send arbitrary lua data to our docking partner */
int lua_send_data(lua_State* L);

/* As a base, build a new empty ship running the default bios. */
int lua_build_ship(lua_State* L);

/* Pew pew pew */
int lua_fire(lua_State* L);

/* Get stuff from the ground */
int lua_mine(lua_State* L);

/* --- QUERIES - with which entities get information about their surroundings */
/* Search the closest entity in the search radius matchin the filter.
 */
int lua_get_entities(lua_State *L);

extern int lua_find_closest(lua_State *L);

/* Get the player id of an entity (or yourself) */
int lua_get_player(lua_State* L);

/* Get the position (x- and y-coordinate) of an entity (or yourself) */
int lua_get_position(lua_State* L);

/* Get your current docking partner (or nil, if not docked) */
int lua_get_docking_partner(lua_State* L);

/* Get the distance of another entity */
int lua_get_distance(lua_State* L);

/* Determine wether you're busy (waiting for some action to finish */
int lua_busy(lua_State* L);

/* Determine whether you're flying or stationary */
int lua_flying(lua_State* L);

/* Get the slot contents of an entity */
int lua_get_slots(lua_State* L);

/* Get the world size */
int lua_get_world_size(lua_State* L);

/* --- Debugging and administrative functions --- */
/* Registers all lua_functions available through c-api */
void register_lua_functions(entity_t *s);

/* Debugging helper function, creating a string description of the given entity */
extern int lua_entity_to_string(lua_State* L);

/* Override lua's "print" function, so that output is not going to a terminal,
 * but rather the player's console buffer */
int lua_print(lua_State* L);

#endif /* _LUAFUNCS_H */
