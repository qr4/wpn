/*
 * Config-File reader (lua as well, of course)
 */
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include "config.h"

lua_State* config_state = NULL;

/* Initialize Configuration by reading a config file */
int init_config_from_file(char* filename) {

	/* warn if we already had a config */
	if(config_state) {
		CONFIG_WARNING("Warning: config_state already exists, overriding with new config from %s (possible memleak of old state?)\n", filename);
		lua_close(config_state);
	}

	/* Create the lua state */
	config_state = luaL_newstate();
	if(config_state == NULL) {
		fprintf(stderr, "Creating configuration lua state failed.\n");
		exit(1);
	}

	/* Load all libraries (All of them, we trust our config files) */
	luaL_openlibs(config_state);

	/* Run the file's code */
	if(luaL_dofile(config_state, filename)) {
		fprintf(stderr, "Warning: Parsing config file %s failed:\n%s\n",filename, lua_tostring(config_state, -1));
		return 0;
	}

	/* Return "true" */
	return 1;
}

/* Get an integer value from the config */
int config_get_int(char* param_name) {
	
	int i;
	lua_getglobal(config_state, param_name);

	/* If this parameter is no int, return 0 */
	if(!lua_isnumber(config_state,-1)) {
		lua_pop(config_state, 1);
		CONFIG_DEBUG("Config parameter %s is not an int, returning 0\n", param_name);
		return 0;
	}

	/* Otherwise, return it's value */
	i = lua_tonumber(config_state, -1);
	lua_pop(config_state, 1);

	return i;
}


/* Get a string value from the config */
const char* config_get_string(char* param_name) {
	const char* s;
	lua_getglobal(config_state, param_name);

	/* If this parameter is no int, return 0 */
	if(!lua_isstring(config_state,-1)) {
		lua_pop(config_state, 1);
		CONFIG_DEBUG("Config parameter %s is not a string, returning \"\"\n", param_name);
		return 0;
	}

	/* Otherwise, return it's value */
	s = lua_tostring(config_state, -1);
	lua_pop(config_state, 1);

	return s;

}

/* Free the configuration */
void free_config() {
	if (config_state != NULL) {
		lua_close(config_state);
		config_state = NULL;
	}
}
