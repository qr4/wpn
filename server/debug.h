#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define ERROR(...) fprintf (stderr, __VA_ARGS__)

#ifdef ENABLE_DEBUG
#define DEBUG(...) printf  (__VA_ARGS__)
#define asprintf(...) if(asprintf(__VA_ARGS__) < 0) ERROR("asprintf failed in " __FILE__ ", %d", __LINE__)
#else
#define DEBUG(...) do {} while(0)
#define asprintf(...) if(asprintf(__VA_ARGS__))
#endif

#endif  /*DEBUG_H*/
