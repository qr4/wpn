/*
 * Config-File reader (lua as well, of course)
 */
#define _GNU_SOURCE
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "config.h"


lua_State* config_state = NULL;
char* config_filename;

OPTIONS_T OPTIONS = {NULL};

int config(int argc, char *argv[]) {
	const struct option options[] = {
		{"lua"    , required_argument , NULL , 'l'} ,
		{"config" , required_argument , NULL , 'c'} ,
		{"init"   , required_argument , NULL , 'i'} ,
		{"help"   , no_argument       , NULL , 'h'} ,
		{0        , 0                 , 0    , 0}
	};

	int c;
	int lines = 0;
	config_filename = strdup("config.lua");
	char *init_filename   = strdup("init.lua");
	char **lua_lines = NULL;

	free_config();

	while ((c = getopt_long(argc, argv, ":l:c:i:h", options, NULL)) != -1) {
		switch (c) {
			case 'l' : // Add Luacode to the config
				printf("Adding \"%s\" to config after config file is loaded.\n", optarg);
				lua_lines = realloc(lua_lines, sizeof(char *) * ++lines);
				lua_lines[lines - 1] = strdupa(optarg);
				break;
			case 'c' : // Set new config-filename
				printf("Setting new config file: %s\n", optarg);
				free(config_filename);
				config_filename = strdup(optarg);
				break;
			case 'i' : // Set new init-file
				printf("Setting new init file: %s\n", optarg);
				free(init_filename);
				init_filename = strdup(optarg);
				break;
			case 'h' :
				fprintf(stderr, 
						"Syntax: %s [-c configfile] [-i initcode] [-h] [-l \"lua code\"...]\n"
						"Where:\n  -c specifies the path of the configfile to use\n"
						"  -i specifies the path of the player-independent spaceship-bios code\n"
						"  -l allows the execution of arbitrary lua code in the config file context\n"
						"  -h prints this help.\n", argv[0]);
				free(init_filename);
				free(lua_lines);
				exit(0);
			case ':' :
				ERROR("Missing argument\n");
				break;
			default :
				// Unknown option or missing param
				break;
		}
	}

	if (!init_config_from_file(config_filename)) {
		ERROR("Could not load config file!\n");
		exit(1);
	}

	for (int i = 0; i < lines; i++) {
		if (luaL_dostring(config_state, lua_lines[i]) == 1) {
			ERROR("Error interpreting \"%s\"\n", lua_lines[i]);
		}
	}

	// Set the path for the bootloader
	if (init_filename) {
		lua_pushstring(config_state, init_filename);
		lua_setglobal(config_state, "ship_init_code_file");
	}

	OPTIONS.config_file = config_filename;
	free(init_filename);
	free(lua_lines);

	return 0;
}

/* Initialize Configuration by reading a config file */
int init_config_from_file(char* filename) {

	/* warn if we already had a config */
	if(config_state) {
		DEBUG("Warning: config_state already exists, overriding with new config from %s (possible memleak of old state?)\n", filename);
		lua_close(config_state);
	}

	/* Create the lua state */
	config_state = luaL_newstate();
	if(config_state == NULL) {
		ERROR("Creating configuration lua state failed.\n");
		exit(1);
	}

	/* Load all libraries (All of them, we trust our config files) */
	luaL_openlibs(config_state);

	/* Run the file's code */
	if(luaL_dofile(config_state, filename)) {
		ERROR("Warning: Parsing config file %s failed:\n%s\n",filename, lua_tostring(config_state, -1));
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
		DEBUG("Config parameter %s is not an int, returning 0\n", param_name);
		return 0;
	}

	/* Otherwise, return it's value */
	i = lua_tonumber(config_state, -1);
	lua_pop(config_state, 1);

	return i;
}

double config_get_double(char* param_name) {

	double d;
	lua_getglobal(config_state, param_name);

	/* If this parameter is no int, return 0 */
	if(!lua_isnumber(config_state,-1)) {
		lua_pop(config_state, 1);
		DEBUG("Config parameter %s is not a double, returning 0\n", param_name);
		return 0;
	}

	/* Otherwise, return it's value */
	d = lua_tonumber(config_state, -1);
	lua_pop(config_state, 1);

	return d;
}

/* Get a string value from the config */
const char* config_get_string(char* param_name) {
	const char* s;
	lua_getglobal(config_state, param_name);

	/* If this parameter is no int, return 0 */
	if(!lua_isstring(config_state,-1)) {
		lua_pop(config_state, 1);
		DEBUG("Config parameter %s is not a string, returning \"\"\n", param_name);
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
	free(OPTIONS.config_file);

	OPTIONS = (OPTIONS_T) {NULL};
}
