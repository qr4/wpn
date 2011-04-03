
#ifndef __NET_H
#define __NET_H

#include <stddef.h>

// hiemit dem net-dinx map-informationen mitteilen
int map_printf(const char *fmt, ...);
// aufrufen, wenn erstmal keine weiteren map-dinge kommen (json zuende)
void map_flush();

// gleiches spiel, nur fuer update-infos
int update_printf(const char *fmt, ...);
// erstmal keine weiteren update-infos -- evtl. will man das gar nicht
// nochmal drueber nachdenken
void update_flush();


// interne
struct net_map_info {
  int pipefd[2];      // pipe zur kommunikation papa <-> kind
  pid_t ppid, cpid;   // papa-pid, kind-pid

  int map_fd[2];      // fds der shm-dinger
  int current_map;    // zeigt auf den gerade zu befuellenden map_fd
};


#endif


