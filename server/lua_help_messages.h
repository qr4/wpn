#ifndef LUA_HELP_MESSAGES_H
#define LUA_HELP_MESSAGES_H
/* This file contains help messages for all available luafunctions (hopefully) */

static const char killself_help[] = 
"\n"
"killself()\n"
"\n"
"Raises a lua error, which cascades through your call stack, overloads your\n"
"ship computer and sends your ship adrift. Needless to say, this is only\n" 
"seldomly a useful function to call by yourself.\n"
"\n"
"Might be useful for other purposes though. (See send_data())\n"
"\n"
"Return value: Are you kidding?\n"
"\n"
"";

static const char moveto_help[] = 
"\n"
"moveto(x,y)\n"
"\n"
"Tells the dumb autopilot to fly your ship to the specified coordinates. This\n"
"autopilot simply accelerates in a straight line, and comes to a halt at the\n"
"specified location. Note that in the past, serious lithobreaking-related\n"
"injuries have been reported when ignoring the presence of asteroids or planets\n"
"in the way. Just so you've been warned. There is a more sophisticated\n"
"autopilot, you know?\n"
"\n"
"Return value: nil\n"
"\n"
"Callbacks set up: When arriving at your destination,\n"
"on_autopilot_arrived(self) is called.\n"
"\n"
"";

static const char set_autopilot_to_help[] = 
"\n"
"set_autopilot_to(x,y)\n"
"\n"
"Tells the more intelligent autopilot to fly your ship to the specified\n"
"coordinates.\n"
"\n"
"Using the latest in overengineered artificial intelligence (as if that meant\n"
"much), this autopilot will do its best to avoid any asteroids or planets in the\n"
"way, and arrive safely at the requested destination.\n"
"\n"
"Flying with this autopilot is slower than using moveto(x,y), but it will\n"
"certainly cause fewer headaches.\n"
"\n"
"Return value: nil\n"
"\n"
"Callbacks set up: When arriving at your destination, on_autopilot_arrived(self)\n"
"is called.\n"
"\n"
"";

static const char set_timer_help[] = 
"\n"
"set_timer(ticks)\n"
"\n"
"Tells your ship to look busy while doing absolutely nothing, for the specified\n"
"number of ticks.\n"
"\n"
"Return value: returns 1 if the timer was set up successfully, or nil if you\n"
"were already genuinely busy.\n"
"\n"
"Callbacks set up: Once the timer expired, on_timer_expired(self) is called.\n"
"\n"
"";

static const char dock_help[] = 
"\n"
"dock(entity)\n"
"\n"
"Dock with the given entity (which must be a ship, base or asteroid). The target\n"
"will have to be within docking range and not currently docked to someone else.\n"
"\n"
"Docking takes some time, during which you are busy. While docked, you can not\n"
"move around.\n"
"\n"
"Note that you can very well dock enemy ships, even as they are shooting at you.\n"
"They might undock() you immediately, though.\n"
"\n"
"Return value: on success, returns the entity you just docked, otherwise nil\n"
"\n"
"Callbacks set up: docking_complete(other) is called with the docking partner's\n"
"entity pointer as an argument, once docking is, well, complete. On the docking\n"
"partner, being_docked(other) is invoked right away.\n"
"\n"
"";

static const char undock_help[] = 
"\n"
"undock()\n"
"\n"
"Detach from your docking partner. You are now free again!\n"
"\n"
"Only works when you are not busy.\n"
"\n"
"Return value: 1 if successful, otherwise nil\n"
"\n"
"Callbacks set up: Upon completion of the undocking,\n"
"on_undocking_complete(other) is called. On the docking partner,\n"
"being_undocked(other) is invoked.\n"
"\n"
"";

static const char transfer_slot_help[] = 
"\n"
"transfer_slot(local_slot, remote_slot)\n"
"\n"
"When docked, transfer a slot from / to your docking partner. More precisely,\n"
"swap the contents of your local slot with that of the remote slot.\n"
"\n"
"Return value: 1 if successful, otherwise nil\n"
"\n"
"Callbacks set up: Upon completion, on_transfer_complete(self) is called.\n"
"\n"
"";

static const char send_data_help[] = 
"\n"
"Transfer lua_data to your docking partner. stuff can be either a string, or a\n"
"lua function (which is automatically dumped as lua bytecode, and also sent as a\n"
"string).\n"
"\n"
"In the lua state of the partner, on_incoming_data(stuff) is invoked. This\n"
"handler can then decide what to do with the string (such as evaluate it, or\n"
"laugh about it and throw it away. If no on_incoming_data() handler is specified\n"
"by the user, a default one is running in a lua state:\n"
"\n"
"function on_incoming_data(d)\n"
"  local f = loadstring(d);\n"
"  f();\n"
"end\n"
"\n"
"Which simply evaluates everything that's thrown at him. Hence, a newly built\n"
"ship can be programmed by it's parent bae through this function.\n"
"\n"
"Return Value: The on_incoming_data() handler on the other side can return a\n"
"single number as return value, which is passed through to the invoking side.\n"
"Otherwise, return value is nil.\n"
"\n"
"Callbacks set up: On the sending side, no callbacks are being invoked. On the\n"
"recieving side, on_incoming_data(stuff) is called to deal with the code.\n"
"\n"
"";

static const char build_ship_help[] = 
"\n"
"build_ship(slot, slot, ...) or build_ship({slot, slot, ...})\n"
"\n"
"Build a ship from a number of ore slots. You need to be a planetary base to do\n"
"this, and not have another ship currently docked to you. Only certain sizes of\n"
"ships are allowed: 3, 6, 12 and 24 slots in size, corresponding to the number\n"
"of arguments supplied to this function.\n"
"\n"
"The resulting new ship will be attached to you as your docking partner, and the\n"
"resource blocks you specified will be consumed.\n"
"\n"
"You typically want to transfer thrusters or weapons into this ship afterwards,\n"
"and load some code into it using send_data()\n"
"\n"
"Return value: The entity ID of the ship you are building, or nil if it failed\n"
"for some reason.\n"
"\n"
"Callbacks set up: once the ship is finished, on_build_complete() is called. The\n"
"newly created ship will only be loaded with the BIOS code, so no callback is\n"
"invoked there.\n"
"\n"
"";

static const char upgrade_base_help[] =
"\n"
"upgrade_base()\n"
"\n"
"Double the size of a planetary base. To do this you need ore in all slots and\n"
"you may neither be busy nor be at the maximum size of 24 yet.\n"
"\n"
"Returns 1 on successful upgrade\n"
"\n"
"Callbacks set up: once the upgrade is finished on_upgrade_complete() is called\n"
"\n"
"";

static const char fire_help[] = 
"\n"
"fire(entity)\n"
"\n"
"Shoot a shot at the specified entity. This only works if you are not otherwise\n"
"busy, the target is in range, and you actually have lasers on board.\n"
"\n"
"A shot has a certain chance of turning a functional slot (such as a laser or\n"
"thruster) into ore on the target ship, or vaporize a chunk of ore. Once\n"
"everything in the target ship has been vaporized, the next shot can detonate\n"
"the ship itself. Hooray for explosions!\n"
"\n"
"After shooting, your weapons need some time to recharge (after which\n"
"on_weapons_ready() will be called for you). The more weapons you have on board,\n"
"the shorter this time becomes. A 24-slot-behemoth completely filled with lasers\n"
"can fire every timestep, and is a fearsome beast indeed.\n"
"\n"
"Return value: nil.\n"
"\n"
"Callbacks set up: Once your weapons have cooled down, on_weapons_ready(self)\n"
"will be called.\n"
"\n"
"";

static const char mine_help[] = 
"\n"
"mine()\n"
"\n"
"As a base, dig one chunk of ore from the planet. This is less efficient (much\n"
"slower) than asteroid mining using spaceships, but the only way for a base to\n"
"produce ore by itself.\n"
"\n"
"You need an empty slot for the ore to be put into.\n"
"\n"
"Return value: the slot number into which the ore was put, or nil if it failed.\n"
"\n"
"Callbacks set up: Once complete, on_mining_complete(self) will be called.\n"
"\n"
"";

static const char manufacture_help[] = 
"\n"
"manufacture(slot, type)\n"
"\n"
"Build something useful out of a chunk of ore, like weapons or thrusters.\n"
"Alternatively, if you have the luxury problem of having too much of those, you\n"
"can turn them back into ore.\n"
"\n"
"Valid arguments for type are DRIVE, WEAPON or ORE.\n"
"\n"
"Return value: on success, the slot number is returned. Otherwise, nil.\n"
"\n"
"Callbacks set up: Once done, on_manufacture_complete(self) will be called.\n"
"\n"
"";

static const char colonize_help[] =
"\n"
"colonize(planet)\n"
"\n"
"As a ship, give up your spatial freedom and turn yourself into a permanent\n"
"settlement. Your ship will lose the ability to move, but gain the possibilities\n"
"to mine, manufacture and build ships.\n"
"\n"
"Return value: your new entity id will be returned, if successful. Otherwise,\n"
"nil.\n"
"\n"
"Callbacks set up: Once the colonization process is complete,\n"
"on_colonize_complete(self) will be called.\n"
"\n"
"";

static const char get_player_help[] = 
"\n"
"get_player(entity) or get_player()\n"
"\n"
"Returns the player id of the specified entity (or yourself).\n"
"\n"
"The entity needs to be within scanner range for this to work.\n"
"\n"
"Return value: The player id number, or nil\n"
"\n"
"";

static const char get_entities_help[] = 
"\n"
"get_entities(range, filter) or get_entities(x, y, range, filter)\n"
"\n"
"Returns a list of entity pointers in the specified range around you or around\n"
"position at (x, y), which match the filter. Valid values for filter are PLANET,\n"
"BASE, SHIP, ASTEROID or any bitwise or'ed combinations of these.\n"
"\n"
"Return value: A list of entities.\n"
"\n"
"";

static const char find_closest_help[] = 
"\n"
"find_closest(range, filter)\n"
"\n"
"Find the closest entity matching the filter in your vicinity, up to a maximum\n"
"range. Valid values for filter are PLANET, BASE, SHIP, ASTEROID or any bitwise\n"
"or'ed combinations of these.\n"
"\n"
"Return value: A single entity pointer, or nil.\n"
"\n"
"";

static const char get_position_help[] = 
"\n"
"get_position(entity) or get_position()\n"
"\n"
"Returns the x and y coordinate of the given entity or self if no argument.\n"
"\n"
"The entity has to be within scanner range for this to work.\n"
"\n"
"Return value: x, y of the target entity, or nil if it is out of range\n"
"\n"
"";

static const char get_distance_help[] = 
"\n"
"get_distance(entity)\n"
"\n"
"Returns the distance to the given entity.\n"
"\n"
"The entity has to be within scanner range for this to work.\n"
"\n"
"Return value: distance to the target enitty, or nil if it is dead/out of range.\n"
"\n"
"";

static const char get_collision_distance_help[] = 
"\n"
"get_collision_distance(entity)\n"
"\n"
"Returns the collision distance to the given entity.\n"
"\n"
"The entity can be out of scanner range.\n"
"\n"
"Return value: collision distance to the target enitty,\n"
"or nil if it is dead/out of range.\n"
"\n"
"";

static const char get_docking_partner_help[] =
"\n"
"get_docking_partner()\n"
"\n"
"Gives you a entity pointer to your docking partner, or nil of you are not\n"
"docked.\n"
"\n"
"Return value: just that.\n"
"\n"
"";

static const char is_busy_help[] = 
"\n"
"is_busy(entity) or is_busy()\n"
"\n"
"Determine whether the given entity, or yourself, is currently busy doing\n"
"something.\n"
"\n"
"Being busy prevents you from performing most other actions (such as shooting,\n"
"building, docking or transferring). Note that waiting for a timer also counts\n"
"as being busy.\n"
"\n"
"The given entity needs to be within scanner range for this to work.\n"
"\n"
"Return value: One of DOCKING, UNDOCKING, TRANSFER, BUILD, TIMER, MINING, MANUFACTURE,\n"
"COLONIZE, UPGRADE, IDLE. (or nil if out of range)\n"
"\n"
"";

static const char is_flying_help[] = 
"\n"
"Determine whether the given entity, or yourself, currently has an autopilot\n"
"following a flight plan, or whether it is just sitting there.\n"
"\n"
"The given entity needs to be within scanner range for this to work.\n"
"\n"
"Return value: A boolean value. (or nil if out of range)\n"
"\n"
"";

static const char get_slots_help[] = 
"\n"
"get_slots(entity) or get_slots()\n"
"\n"
"Get a list of the slot contents of another entity, or yourself.\n"
"\n"
"The given entity needs to be within scanner range for this to work.\n"
"\n"
"Return value: A list of slot values (EMPTY, ORE, DRIVE or WEAPON). Or nil, if\n"
"out of range.\n"
"\n"
"";

static const char get_world_size_help[] = 
"\n"
"get_world_size()\n"
"\n"
"Return value: 4 numbers: xmin, xmax, ymin, ymax, which describe the extents of\n"
"the game's world.\n"
"\n"
"";

static const char get_type_help[] = 
"\n"
"get_type(entity)\n"
"\n"
"Return value: Returns the type of the given entity pointer. Can be PLANET,\n"
"SHIP, BASE or ASTEROID\n"
"\n"
"";

static const char entity_to_string_help[] = 
"\n"
"entity_to_string(entity)\n"
"\n"
"Gives a string representation of the given entity. Useful for debugging output.\n"
"\n"
"Return value: a string, describing the entity, or nil if it is an invalid,\n"
"out-of-range, or otherwise bad.\n"
"\n"
"";

static const char help_help[] = 
"\n"
"help() or help(\"command\")\n"
"\n"
"Gives interactive help on the lua console. Ideally this will be the same\n"
"information you can find in this wiki here.\n"
"\n"
"Return value: nil. Information will be printed on your lua console.\n"
"\n"
"";

static const char print_help[] = 
"\n"
"print(string)\n"
"\n"
"Allows you to print debugging information into your lua log buffer, which you\n"
"can access via the console.\n"
"\n"
"Hooray for watching shit scroll by.\n"
"\n"
"Return value: nil. The given string will be printed in the lua log buffer.\n"
"\n"
"";

#endif  /*LUA_HELP_MESSAGES_H*/
