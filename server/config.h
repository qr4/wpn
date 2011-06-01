#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdio.h>
#include "debug.h"

int init_config_from_file(char* filename);
int config_get_int(char* param_name);
const char* config_get_string(char* param_name);
void free_config();

#endif /* _CONFIG_H */
