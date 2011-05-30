#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define ERROR(...) fprintf (stderr, __VA_ARGS__)

#ifdef ENABLE_DEBUG
#define DEBUG(...) printf  (__VA_ARGS__)
#else
#define DEBUG(...) do {} while(0)
#endif

#endif  /*DEBUG_H*/
