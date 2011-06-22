#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <lauxlib.h>
#include "luafuncs.h"
#include "luastate.h"
#include "entities.h"
#include "ship.h"
#include "storages.h"
#include "types.h"
#include "json_output.h"
#include "debug.h"
#include "config.h"
#include "route.h"
#include "../net/talk.h"

/* Type of functions which we make available to lua states */
typedef struct {
	lua_CFunction c_function;
	const char lua_function_name[32];
} lua_function_entry;

/* Add all functions that shall be available for the shipcomputers here */
static const lua_function_entry lua_wrappers[] = {
//   C-function name,      Lua-function name

	/* Action */
	{lua_killself,         "killself"},
	{lua_moveto,           "moveto"},
	{lua_set_autopilot_to, "set_autopilot_to"},
	{lua_set_timer,        "set_timer"},
	{lua_dock,             "dock"},
	{lua_undock,           "undock"},
	{lua_transfer_slot,    "transfer_slot"},
	{lua_send_data,        "send_data"},
	{lua_build_ship,       "build_ship"},
	{lua_fire,             "fire"},
	{lua_mine,             "mine"},

	/* Queries */
	{lua_entity_to_string,    "entity_to_string"},
	{lua_get_player,          "get_player"},
	{lua_get_entities,        "get_entities"},
	{lua_find_closest,        "find_closest"},
	{lua_get_position,        "get_position"},
	{lua_get_distance,        "get_distance"},
	{lua_get_docking_partner, "get_docking_partner"},
	{lua_busy,                "is_busy"},
	{lua_flying,              "is_flying"},
	{lua_get_slots,           "get_slots"},
	{lua_get_world_size,      "get_world_size"},
	{lua_get_type,         "get_type"},

	/* More lowlevel stuff */
	{lua_print,            "print"},
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
 * 	Return value: none.
 */
int lua_stop(lua_State* L) {
	entity_t* e;
	entity_id_t id;
	int n;

	/* Check number of arguments */
	n = lua_gettop(L);
	if(n > 0) {
			lua_pushstring(L, "Wrong number of arguments to stop.");
			lua_error(L);
	}

	id = get_self(L);
	e = get_entity_by_id(id);

	/* Now call the stop-flightplanner */
	stop_planner(e);

	return 0;
}

/* Fly straight to the specified coordinates, ignoring any object in the way
 * Arguments: x, y[, on_arrival]
 * where:
 * 				x: x-coordinate of target location
 * 				y: y-coordinate of target location
 * 	Return value: none.
 */
int lua_moveto(lua_State* L) {
	entity_t* e;
	entity_id_t id;
	int n;
	double x = 0;
	double y = 0;

	/* Check number of arguments */
	n = lua_gettop(L);
	switch(n) {
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

	id = get_self(L);
	e = get_entity_by_id(id);

	/* If we are docked, or if we are a base, we can't move. */
	if(e->ship_data->docked_to.id != INVALID_ID.id || e->type == BASE) {
		return 0;
	}

	/* Bounds-check the coordinates */
	if(x < map.left_bound || x > map.right_bound || y < map.upper_bound || y > map.lower_bound) {
		return 0;
	}

	/* Now call the moveto-flightplanner */
	moveto_planner(e, x, y);

	return 0;
}

/* Fly to the specified coordinates, using the intelligent autopilot
 * Arguments: x, y[, on_arrival]
 * where:
 * 				x: x-coordinate of target location
 * 				y: y-coordinate of target location
 * 	Return value: none.
 */
int lua_set_autopilot_to(lua_State* L) {
	entity_t* e;
	entity_id_t id;
	int n;
	double x=0,y=0;

	/* Check number of arguments */
	n = lua_gettop(L);
	switch(n) {
		case 2:

			/* Get coordinates */
			if(!lua_isnumber(L,-1) || !lua_isnumber(L,-2)) {
				lua_pushstring(L, "coordinates to set_autopilot_to must be numeric");
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

	id = get_self(L);
	e = get_entity_by_id(id);

	/* If we are docked, or if we are a base, we can't move. */
	if(e->ship_data->docked_to.id != INVALID_ID.id || e->type == BASE) {
		return 0;
	}

	/* Bounds-check the coordinates */
	if(x < map.left_bound || x > map.right_bound || y < map.upper_bound || y > map.lower_bound) {
		return 0;
	}

	/* Now call the flightplanner */
	autopilot_planner(e, x, y);

	return 0;
}

/* Dock with another entity */
int lua_dock(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	if(n!=1 || !lua_islightuserdata(L,-1)) {
		lua_pushstring(L, "lua_dock must be called with exactly one entity argument");
		lua_error(L);
	}

	/* Get self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Get the docking target */
	id.id = (uint64_t) lua_touserdata(L,-1);
	lua_pop(L,1);
	e = get_entity_by_id(id);

	/* Check that we are not trying to dock ourselves, because that's bullshit! */
	if(id.id == self.id) {
		DEBUG("Trying to dock self!\n");
		return 0;
	}

	DEBUG("Attempting dock...");

	/* Check that we are targetting something dockable */
	if(e->type & ~(SHIP|BASE|ASTEROID)) {
		DEBUG("Not dockable!\n");
		return 0;
	}

	/* Check that neither we, nor our target are currently docked to someone else */
	if(eself->ship_data->docked_to.id != INVALID_ID.id || e->ship_data->docked_to.id != INVALID_ID.id) {
		DEBUG("Already docked!\n");
		return 0;
	}

	/* Check that we are not currently waiting for some other action to complete */
	if(is_busy(eself)) {
		DEBUG("Currently waiting for timer!\n");
		return 0;
	}

	/* Check that we're within docking range */
	if(dist(eself,e) > config_get_double("docking_range")) {
		DEBUG("Out of range!\n");
		return 0;
	}

	/* TODO: Check for speed difference */

	/* Perform the actual docking */
	eself->ship_data->docked_to.id = id.id;
	e->ship_data->docked_to.id = self.id;

	/* Set up timer to inform the client once docking is complete */
	set_entity_timer(eself, config_get_int("docking_duration"), DOCKING_COMPLETE, id);

	/* Inform the other entity that it is being docked */
	call_entity_callback(e, BEING_DOCKED, self);

	/* Return one in order to denote successful initialization of docking process */
	fprintf(stderr, "Success!\n");
	lua_pushnumber(L,1);
	return 1;
}

/* Undock from your docking partner */
int lua_undock(lua_State* L) {
	entity_id_t self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	if(n!=0) {
		lua_pushstring(L, "Invalid number of arguments: undock() doesn't need any.");
		lua_error(L);
	}

	/* Get self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Determine that we are, in fact, docked */
	if(eself->ship_data->docked_to.id == INVALID_ID.id) {

		/* If not, just return nil */
		return 0;
	}

	/* Make certain that our docking partner hasn't exploded... or worse. */
	e = get_entity_by_id(eself->ship_data->docked_to);
	if(!e) {
		return 0;
	}

	/* Check that we are not currently waiting for some other action to complete */
	if(is_busy(eself)) {
		DEBUG("Can't undock: currently waiting for timer!\n");
		return 0;
	}

	/* Remove the dockedness. */
	eself->ship_data->docked_to.id = INVALID_ID.id;
	e->ship_data->docked_to.id = INVALID_ID.id;

	/* inform the other entity that it is being undocked */
	call_entity_callback(e, BEING_UNDOCKED, self);

	/* Set up timer to inform the client once undocking is complete */
	set_entity_timer(eself, config_get_int("undocking_duration"), UNDOCKING_COMPLETE, e->unique_id);

	/* Return 1 to denote successful undocking */
	lua_pushnumber(L,1);

	return 1;
}

/* Pew pew pew! */
int lua_fire(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;
	unsigned int slot, num_occ_slots, i;
	unsigned int num_lasers = 0;
	double hit_probability;

	n = lua_gettop(L);

	if(n!=1 || !lua_islightuserdata(L,-1)) {
		lua_pushstring(L, "fire must be called with exactly one entity argument");
		lua_error(L);
	}

	/* Get self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Get the target */
	id.id = (uint64_t) lua_touserdata(L,-1);
	lua_pop(L,1);
	e = get_entity_by_id(id);

	/* Check that we are not trying to shoot ourselves, because that's bullshit! */
	if(id.id == self.id) {
		DEBUG("Trying to shoot self! Excuse me, have you lost your marbles?\n");
		return 0;
	}

	/* Check that we're not currently busy, or our lasers reloading. */
	if(is_busy(eself)) {
		return 0;
	}

	/* Check that we are targetting something shootable and determine it's
	 * hit probability */
	switch(e->type) {
		case SHIP:
			hit_probability = config_get_double("ship_hit_probability");
			break;
		case BASE:
			hit_probability = config_get_double("base_hit_probability");
			break;
		case ASTEROID:
			/* Why would anyone want to shoot asteroids anyway?
			 * Well, what do we care. */
			hit_probability = config_get_double("asteroid_hit_probability");
			break;
		default:
			DEBUG("Not shootable\n");
			return 0;
	}


	/* Record the shot in a json update */
	current_shots = realloc(current_shots, sizeof(char*) * (n_current_shots+2));
	if(!current_shots) {
		ERROR("Out of memory for shot json-strings.\n");
		return 0;
	}

	current_shots[n_current_shots++] = shot_to_json(eself, e);
	current_shots[n_current_shots] = NULL;

	/* Yay, let's break something! */
	if((double)rand() / RAND_MAX < hit_probability) {

		/* Select a random occupied slot on the target */
		num_occ_slots = 0;
		/* Count occupied slots */
		for(i=0; i < e->slots; i++) {
			if(e->slot_data->slot[i] != EMPTY) {
				num_occ_slots++;
			}
		}

		/* oh look, this ship is empty. We actually destroy it. */
		if(num_occ_slots == 0) {
			/* Bam! */
			explode_entity(e);

			/* Now we are busy recharging our lasers */
			/* Count lasers */
			for(i=0; i<eself->slots; i++) {
				if(eself->slot_data->slot[i] == WEAPON) {
					num_lasers++;
				}
			}
			set_entity_timer(eself, config_get_int("laser_recharge_duration")/num_lasers, WEAPONS_READY, self);

			return 0;
		}

		/* Otherwise we select a slot */
		i = rand() % num_occ_slots;
		for(slot=0; slot < e->slots; slot++) {
			if(e->slot_data->slot[slot] != EMPTY) {
				i--;
				if(i<0)
					break;
			}
		}

		/* Functional components are turned into ore */
		if(e->slot_data->slot[slot] != ORE) {
			e->slot_data->slot[slot] = ORE;
		} else {
			/* Ore on the other hand, is vaporized */
			e->slot_data->slot[slot] = EMPTY;
		}

	}

	/* Invoke "oh noes I've been hit" notifier on the other side */
	call_entity_callback(e, SHOT_AT, self);

	/* TODO: Do we have to nudge the autopilot again? */

	/* Now we are busy recharging our lasers */
	/* Count lasers */
	for(i=0; i<eself->slots; i++) {
		if(eself->slot_data->slot[i] == WEAPON) {
			num_lasers++;
		}
	}
	set_entity_timer(eself, config_get_int("laser_recharge_duration")/num_lasers, WEAPONS_READY, self);

	/* TODO: Return whether we hit? */
	return 0;
}

/* Let a base get one resource chunk from a planet */
int lua_mine(lua_State *L) {
	entity_id_t self;
	entity_t* eself;
	int slot;
	int n;

	n = lua_gettop(L);
	if(n!=0) {
		lua_pushstring(L, "mine doesn't take any arguments");
		lua_error(L);
	}

	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Only bases can mine */
	if(eself->type != BASE) {
		return 0;
	}

	/* We won't mine if we're busy */
	if(is_busy(eself)) {
		return 0;
	}


	/* Find a free slot */
	for(slot=0; slot < eself->slots; slot++) {
		if(eself->slot_data->slot[slot] == EMPTY) {
			break;
		}
	}

	if(slot == eself->slots) {
		/* No slot free -> no mining */
		return 0;
	}

	/* Fill it in */
	eself->slot_data->slot[slot] = ORE;

	/* Set the timer. */
	set_entity_timer(eself, config_get_int("mining_duration"), MINING_COMPLETE, self);

	/* Return the slot number we mine into. */
	lua_pushnumber(L, slot+1);
	return 1;
}

/* 
 * Returns all object within a search area. 
 * Moving objects are masked out it out of scanner-range
 */
int lua_get_entities(lua_State *L) {
	entity_id_t self;
	entity_id_t *found;
	size_t n_found;
	size_t i;
	vector_t search_center;
	double search_radius = 0;
	unsigned int filter = 0;;
	int n;

	n = lua_gettop(L);

	self = get_self(L);

	switch (n) {
		// search from the entity position
		case 2 :
			search_center = get_entity_by_id(self)->pos;
			filter        = (unsigned int) lua_tonumber(L, -1);
			search_radius = (double)       lua_tonumber(L, -2);
			lua_pop(L, 2);
			break;
		case 4 :
			filter          = (unsigned int) lua_tonumber(L, -1);
			search_radius   = (double)       lua_tonumber(L, -2);
			search_center.y = (double)       lua_tonumber(L, -3);
			search_center.x = (double)       lua_tonumber(L, -4);
			lua_pop(L, 4);
			break;
		default :
			lua_pushstring(L, 
					"Invalid number of arguments for get_entities()!\n"
					"valid calls are:\n"
					"get_entities(radius, filter)\n"
					"get_entities(x, y, radius, filter)\n");
			lua_error(L);
			break;
	}

	found = get_entities(search_center, search_radius, filter, &n_found);
	
	lua_newtable(L);
	n = 0;

	for (i = 0; i < n_found; i++) {
		if (in_scanner_range(self, found[i])) {
			n++;
			lua_pushnumber(L, n);                           // key
			lua_pushlightuserdata(L, (void *) found[i].id); // valua
			lua_settable(L, -3);                            // add to table
		}
	}
	
	free(found);

	return 1;
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
	entity_t* closest;

	n = lua_gettop(L);

	if (n != 2) {
		lua_pushstring(L, "Invalid number of arguments for find_closest()!");
		lua_error(L);
	}

	filter        = (unsigned int) lua_tonumber(L, -1);
	search_radius = (double) lua_tonumber(L, -2);
	lua_pop(L, 2);
	id = get_self(L);
	e = get_entity_by_id(id);

	closest = find_closest(e, search_radius, filter);

	if(closest != NULL) {
		lua_pushlightuserdata(L, (void*)(closest->unique_id.id));

		return 1;
	} else {
		return 0;
	}
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
	if(n != 1) {
		lua_pushstring(L, "entity_to_string expects exactly one entity pointer\n");
		lua_error(L);
	}

	/* If the thing that was passed to us is not an entity pointer, our string is also null. */
	if(!lua_islightuserdata(L,1)) {
		return 0;
	}

	/* Pop entity pointer from stack */
	id.id = (uint64_t) lua_touserdata(L, -1);
	lua_pop(L,1);
	e = get_entity_by_id(id);

	if (e == NULL) {
		lua_pushstring(L, "nil");
	} else {
		/* Fill in description */
		temp = slots_to_string(e);
		asprintf(&s, "{\n\tEntity %lu:\n\tpos: %f,  %f\n\tv: %f, %f\n\ttype: %s\n\tslots: %i\n\tplayer: %i\n\tcontents: [%s]\n}\n",
					(size_t)e->unique_id.id, e->pos.x, e->pos.y, e->v.x, e->v.y, type_string(e->type), e->slots, e->player_id, temp);

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
		lua_pushstring(L, "set_timer expects exactly one integer (number of ticks to wait)");
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

	if(is_busy(e)) {
		/* Another timer is already ticking for this entity, so we don't set one.
		 * Return 0 to inform the ship.*/
		lua_pushnumber(L,0);
		return 1;
	}

	/* Setup the actual timer */
	set_entity_timer(e, n, TIMER_EXPIRED, id);

	/* Return 1 */
	lua_pushnumber(L,1);
	return 1;
}

/* Get the player id of an entity (or yourself) */
int lua_get_player(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	/* Get self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	switch(n) {
		case 0: /* No argument -> get own player ID */
			lua_pushnumber(L,eself->player_id);
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
				/* Check that the target entity is within scanner range */
				if(dist(eself, e) < config_get_double("scanner_range")) {
					lua_pushnumber(L,e->player_id);
					return 1;
				} else {
					return 0;
				}
			}
		default :
			lua_pushstring(L, "get_player expects exactly one entity argument");
			lua_error(L);
			return 0;
	}
}

/* Get the position (x- and y-coordinate) of an entity (or yourself) */
int lua_get_position(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	/* Set self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	switch(n) {
		case 0: /* No argument -> get own position */
			lua_pushnumber(L,eself->pos.x);
			lua_pushnumber(L,eself->pos.y);
			return 2;
		case 1: /* One argument -> get ID of target */
			if(!lua_islightuserdata(L,-1)) {
				lua_pushstring(L, "Argument to get_position was not an entity pointer!");
				lua_error(L);
			}

			id.id = (uint64_t) lua_touserdata(L,-1);
			lua_pop(L,1);

			e = get_entity_by_id(id);
			if(e == NULL) {
				/* Return nil if the entity doesn't exist */
				return 0;
			} else {
				/* Check that the target entity is within scanner range */
				if(dist(eself, e) < config_get_double("scanner_range")) {
					lua_pushnumber(L,e->pos.x);
					lua_pushnumber(L,e->pos.y);
					return 2;
				} else {
					return 0;
				}
			}
		default :
			lua_pushstring(L, "get_position expects exactly one entity argument");
			lua_error(L);
			return 0;
	}

}

/* Get your current docking partner (or nil, if not docked) */
int lua_get_docking_partner(lua_State* L) {

	entity_id_t self;
	entity_t *eself;
	int n;

	n = lua_gettop(L);

	/* Syntax check */
	if(n!=0) {
		lua_pushstring(L, "Invalid number of arguments: get_docking_partner() doesn't need any.");
		lua_error(L);
	}

	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Check whether we're docked */
	if(eself->ship_data->docked_to.id == INVALID_ID.id) {
		return 0;
	} else {
		DEBUG("Docked: %lu <-> %lu\n", eself->unique_id.id, eself->ship_data->docked_to.id);

		/* Return our docking partner's id */
		lua_pushlightuserdata(L, (void*)(eself->ship_data->docked_to.id));
		return 1;
	}
}

/* Get the distance of another entity */
int lua_get_distance(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	/* Set self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	if(n != 1 || !lua_islightuserdata(L,-1)) {
			lua_pushstring(L, "get_distance expects exactly one entity argument");
			lua_error(L);
	}

	/* Get target */
	id.id = (uint64_t) lua_touserdata(L,-1);
	lua_pop(L,1);
	e = get_entity_by_id(id);
	if(e == NULL) {
		/* Return nil if the entity doesn't exist */
		return 0;
	} else {
		/* Check that the target entity is within scanner range */
		if(dist(eself, e) < config_get_double("scanner_range")) {

			/* Return its distance */
			lua_pushnumber(L,dist(eself, e));
			return 1;
		} else {
			return 0;
		}
	}
}

/* Transfer a slot from / to the docked partner */
/* Arguments are: local_slot and remote_slot.
 * Only works when docked. */
int lua_transfer_slot(lua_State* L) {
	entity_id_t self;
	entity_t *e, *eself;
	int n;
	int local_slot, remote_slot;

	n = lua_gettop(L);

	if(n != 2 || !lua_isnumber(L,-1) || !lua_isnumber(L,-2)) {
		lua_pushstring(L, "transfer_slot() requires exactly two number arguments.");
		lua_error(L);
	}

	local_slot = lua_tonumber(L,1);
	remote_slot = lua_tonumber(L,2);
	lua_pop(L,2);

	/* get self */
	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Check whether we're docked */
	if(eself->ship_data->docked_to.id == INVALID_ID.id) {
		return 0;
	}

	/* Check that we are not currently waiting for some other action to complete */
	if(is_busy(eself)) {
		DEBUG("Not transferring: currently waiting for timer!\n");
		return 0;
	}

	e = get_entity_by_id(eself->ship_data->docked_to);
	if(!e) {
		/* Looks like our docking partner has exploded. Poor guy. */
		return 0;
	}

	/* Swap the slots */
	if(swap_slots(eself, local_slot, e, remote_slot) != OK) {

		/* Somethings has gone wrong. */
		return 0;
	}

	/* Now the little robognomes on board are busy carrying over the chests. */
	set_entity_timer(eself, config_get_int("transfer_duration"), TRANSFER_COMPLETE, self);

	/* Return 1 to indicate successful initiation of transfer */
	lua_pushnumber(L,1);
	return 1;
}

/* Function_dump writer for transferral of data between lua states */
static int data_transfer_writer (lua_State *L, const void* b, size_t size, void* B) {
	luaL_addlstring((luaL_Buffer*) B, (const char *)b, size);
	return 0;
}

/* Send arbitrary lua data to your docking partner */
int lua_send_data(lua_State* L) {
	entity_id_t self, partner;
	entity_t* eself, *epartner;
	int n = lua_gettop(L);
	luaL_Buffer b;

	if(n != 1) {
		lua_pushstring(L, "Invalid Arguments: send_data expects exactly one argument");
		lua_error(L);
	}

	/* Get self */
	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Determine that we're docked to someone */
	partner = eself->ship_data->docked_to;
	if(partner.id == INVALID_ID.id) {
		return 0;
	}
	epartner = get_entity_by_id(partner);
	if(!epartner) {
		/* Looks like our docking partner vanished. */
		return 0;
	}

	/* Check whether the other entity even has a lua-state, and whether that one
	 * can handle incoming data */
	if(!(epartner->lua)) {
		return 0;
	}
	lua_getglobal(epartner->lua, "on_incoming_data"); /* TODO: Don't hardcode this name */
	if(lua_isnil(epartner->lua, -1)) {
		lua_pop(epartner->lua, 1);
		return 0;
	}

	/* Create a stringbuffer in the other entities' lua state, and serialize our data into it */
	luaL_buffinit(epartner->lua, &b);
	if(lua_dump(L, data_transfer_writer, &b)) {
		DEBUG("Error while dumping lua data\n");
		return 0;
	}
	luaL_pushresult(&b);

	/* We can now savely pop our argument */
	lua_pop(L,1);

	/* Call the handler function on the other side */
	lua_active_entity = partner;
	if(lua_pcall(epartner->lua, 1,1,0)) {
		/* An error occured in the other state.
		 * Let's be fair and just ignore it. */
		DEBUG("Error sending data to docking partner: %s\n", lua_tostring(epartner->lua, -1));
		lua_pop(epartner->lua,1);
		lua_pushnil(L);
	} else {

		/* The code on the other side may return a number to us */
		if(lua_isnumber(epartner->lua, -1)) {
			lua_pushnumber(L, lua_tonumber(epartner->lua,-1));
			lua_pop(epartner->lua,1);
		} else {
			/* Whatever he may have returned, we don't want it. */
			lua_pop(epartner->lua,1);
			lua_pushnil(L);
		}
	}

	lua_active_entity = self;
	return 1;
}

/* Determine whether you are currently waiting for some action to finish */
int lua_busy(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	/* Set self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	switch(n) {
		case 0:
			/* Get your own id */
			id = self;
			e = eself;
			break;
		case 1:
			/* Get id of someone else */
			if(!lua_islightuserdata(L,-1)) {
				lua_pushstring(L, "Argument to is_busy is not an entity!");
				lua_error(L);
			}

			id.id = (uint64_t) lua_touserdata(L,-1);
			lua_pop(L,1);

			e = get_entity_by_id(id);

			/* If this entity doesn't exist -> return */
			if(!e) {
				return 0;
			}

			/* Check that entity is within scanner range */
			if(dist(eself, e) > config_get_double("scanner_range")) {
				return 0;
			}

			break;
		default:
			lua_pushstring(L, "is_busy expects no, or one entity argument");
			lua_error(L);
			return 0;
	}

	/* If the requested entity is not a ship or base, it is certainly idle. */
	if(!(e->type & (SHIP|BASE))) {
		lua_pushboolean(L,0);
		return 1;
	}

	/* If no timer is set, we are idle. */
	lua_pushboolean(L,is_busy(e));
	return 1;
}

/* Determine whether another entity, or yourself, is currently flying or stationary */
int lua_flying(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	/* Set self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	switch(n) {
		case 0:
			/* Get your own id */
			id = self;
			e = eself;
			break;
		case 1:
			/* Get id of someone else */
			if(!lua_islightuserdata(L,-1)) {
				lua_pushstring(L, "Argument to is_flying is not an entity!");
				lua_error(L);
			}

			id.id = (uint64_t) lua_touserdata(L,-1);
			lua_pop(L,1);

			e = get_entity_by_id(id);

			/* If this entity doesn't exist -> return */
			if(!e) {
				return 0;
			}

			/* Check that entity is within scanner range */
			if(dist(eself, e) > config_get_double("scanner_range")) {
				return 0;
			}

			break;
		default:
			lua_pushstring(L, "is_flying expects no, or one entity argument");
			lua_error(L);
			return 0;
	}

	/* If the entity is not a ship, it's always stationary */
	if(e->type != SHIP) {
		lua_pushboolean(L,0);
		return 1;
	}

	/* If the autopilot has a NULL waypoint list, the entity is stationary */
	if(e->ship_data->flightplan == NULL) {
		lua_pushboolean(L,0);
		return 1;
	}

	/* otherwise it's flying */
	lua_pushboolean(L,1);
	return 1;
}

/* Return the content of another (or your own) entities' slots */
int lua_get_slots(lua_State* L) {
	entity_id_t id, self;
	entity_t *e, *eself;
	int n;

	n = lua_gettop(L);

	/* Set self entity */
	self = get_self(L);
	eself = get_entity_by_id(self);

	switch(n) {
		case 0:
			/* Get your own contents */
			id = self;
			e = eself;
			break;
		case 1:
			/* Get contents of someone else */
			if(!lua_islightuserdata(L,-1)) {
				lua_pushstring(L, "Argument to get_slots is not an entity!");
				lua_error(L);
			}

			id.id = (uint64_t) lua_touserdata(L,-1);
			lua_pop(L,1);

			e = get_entity_by_id(id);

			/* If this entity doesn't exist -> return */
			if(!e) {
				return 0;
			}

			/* Check that entity is within scanner range */
			if(dist(eself, e) > config_get_double("scanner_range")) {
				return 0;
			}

			break;
		default:
			lua_pushstring(L, "get_slots expects no, or one entity argument");
			lua_error(L);
			return 0;
	}

	lua_newtable(L);

	/* Push each of the slot contents on the stack */
	for(n = 0; n<e->slots; n++) {
		lua_pushinteger(L, n + 1);                // key
		lua_pushnumber(L, e->slot_data->slot[n]); // value
		lua_settable(L, -3);                      // add to table
	}

	/* Return the number of slots, thus forming a valid lua list */
	return 1;
}

/* As a base, build a new empty ship running the default bios.
 * A number of resource slot has to be specified and will be consumed in the process.
 * The new ship will then be the new docking partner */
int lua_build_ship(lua_State* L) {

	entity_id_t id, self;
	entity_t *eself, *e;
	int n,a;
	int used_slots[24];
	vector_t pos;

	n = lua_gettop(L);

	switch(n) {
		case 24:
		case 12:
		case 6:
		case 3:
			break;
		default:
			lua_pushstring(L,"Attempted to build a ship of invalid size.");
			lua_error(L);
			return 0;
	}

	self = get_self(L);
	eself = get_entity_by_id(self);

	/* Check the arguments */
	/* TODO: Should all errors here really be fatal for the ship? */
	for(int i=0; i<n; i++) {
		if(!lua_isnumber(L,i+1)) {
			lua_pushstring(L, "Invalid type for arguments of build: Integer slot indices required");
			lua_error(L);
		}
		
		a = lua_tonumber(L,i+1);

		/* Lua counts from one, lets do that too */
		a-=1;

		/* No slot should be specified twice */
		for(int j=0; j<i; j++) {
			if(a == used_slots[j]) {
				lua_pushstring(L, "A slot was specified twice in build_ship!");
				lua_error(L);
			}
		}

		/* Ships should be build from resource / ore */
		if(eself->slot_data->slot[i] != ORE) {
			lua_pushstring(L, "Attempted to build a ship from a non-resource block.");
			lua_error(L);
		}

		used_slots[i]=a;
	}

	/* Pop all arguments */
	lua_pop(L,n);

	/* Check that we are a base */
	if(!eself->type == BASE) {
		return 0;
	}

	/* Check that we're not currently busy, or docked to someone else */
	if(is_busy(eself)) {
		return 0;
	}
	if(eself->base_data->docked_to.id != INVALID_ID.id) {
		return 0;
	}

	/* So far, everything looks nice. Let's build a ship then! */
	pos.x = eself->pos.x +  config_get_double("build_offset_x");
	pos.y = eself->pos.y + config_get_double("build_offset_y");
	id = init_ship(ship_storage, pos, n);
	e = get_entity_by_id(id);

	e->player_id = eself->player_id;

	/* Zero out the slots we used for building. */
	for(int i=0; i<n; i++) {
		eself->slot_data->slot[used_slots[i]] = EMPTY;
	}

	/* TODO: Currently, the new ship is created instantly. There should be a way
	 * to delay it. */

	/* Return the ship's id to the caller */
	lua_pushlightuserdata(L, (void*)(id.id));
	return 1;
}

/* Push the map extents to the lua state */
int lua_get_world_size(lua_State* L) {
	
	lua_pushnumber(L, map.left_bound);
	lua_pushnumber(L, map.right_bound);
	lua_pushnumber(L, map.upper_bound);
	lua_pushnumber(L, map.lower_bound);
	return 4;
}

/* determine the type of the given entity */
int lua_get_type(lua_State* L) {
	int n;
	entity_id_t id;
	entity_t* e;

	n = lua_gettop(L);
	if(n!=1 || !lua_islightuserdata(L,1)) {
		lua_pushstring(L,"get_type requires one (entity) argument.\n");
		lua_error(L);
	}

	id.id = (uint64_t) lua_touserdata(L,1);
	lua_pop(L,1);

	/* Check whether this entity even exists */
	e = get_entity_by_id(id);
	if(!e) {
		/* A nonexistant entity gives nil */
		return 0;
	}

	/* Push the type and return */
	lua_pushnumber(L,id.type);

	return 1;
}

/* Override lua's "print" function, so that output is not going to a terminal,
 * but rather the player's console buffer */
int lua_print(lua_State* L) {
	int n=lua_gettop(L);
	entity_id_t self;
	entity_t* eself;
	char* s;

	self = get_self(L);
	eself = get_entity_by_id(self);

	for(int i=1; i<=n; i++) {
		asprintf(&s, "%s\n", lua_tostring(L,i));
		talk_log_lua_msg(eself->player_id, s, strlen(s));
		free(s);
	}

	lua_pop(L,n);
	return 0;
}
