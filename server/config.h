#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdio.h>
#include "debug.h"

typedef struct OPTIONS_T OPTIONS_T;

struct OPTIONS_T {
	char *config_file;
};

extern OPTIONS_T OPTIONS;

int config(int argc, char *argv[]);
int init_config_from_file(char* filename);
int config_get_int(char* param_name);
double config_get_double(char* param_name);
const char* config_get_string(char* param_name);
void free_config();

#endif /* _CONFIG_H */
