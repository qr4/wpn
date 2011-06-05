#ifndef _LUASTATE_H
#define _LUASTATE_H

#include <lua.h>
#include "entities.h"

/* Since entity-pointers are just light userdata in lua, nothing prevents a
 * ship from overwriting it's self pointer and claiming to be someone else.
 *
 * To detect this kind of cheating, lua functions should verify that the entity
 * they are dealing with is actually the one currently active. Since no third-party-data can be passed to lua-called functions directly, this will have to be acheived through a global variable.
 * (TODO: If multithreaded, this will have to be thread-local)
 */
extern entity_id_t lua_active_entity;

/* lua callback-function names (defined in luastate.c) */
extern char* callback_names[NUM_EVENTS];

extern void init_ship_computer(entity_t* s);
extern void kill_computer(entity_t* s);

void call_entity_callback(entity_t* e, event_t event);


#endif /* _LUASTATE_H */
