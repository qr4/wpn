#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdio.h>

int init_config_from_file(char* filename);
int config_get_int(char* param_name);
const char* config_get_string(char* param_name);
void free_config();

#define CONFIG_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define CONFIG_WARNING(...) fprintf(stderr, __VA_ARGS__)
#define CONFIG_ERROR(...) fprintf(stderr, __VA_ARGS__)

#endif /* _CONFIG_H */
