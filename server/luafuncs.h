#ifndef _LUAFUNCS_H
#define _LUAFUNCS_H

#include <lua.h>

/* Functions callable from within lua.
 *
 * To make new functions available to the spacecrafts' lua state,
 * also remember to add them in the init_ship_computer function
 * in luastate.c
 */

/* Kill the lua state by throwing an error, sends the spacecraft adrift */
extern int lua_killself(lua_State* L);

/* Fly straight to the specified coordinates */
extern int lua_moveto(lua_State* L);

/* Using the (more) intelligent autopilot, fly to the specified coordinates,
 * avoiding planets and asteroid fields */
extern int lua_set_autopilot_to(lua_State* L);

/* Search the closest entity in the search radius matchin the filter.
 */
extern int lua_find_closest(lua_State *L);

/* Debugging helper function, creating a string description of the given entity */
int lua_entity_to_string(lua_State* L);


#endif /* _LUAFUNCS_H */
