
//
// code by twist<at>nerd2nerd.org
//

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

void net_init();

#if 0
// interne dinge die andere sowieso nicht sehen -- fork ist mein freund

enum net_client_status { 
  NETCS_NOT_CONNECTED,    // hier lauscht niemand
  NETCS_NEW_CONNECTED,    // verbindung wurde gerade aufgebaut, die map muss dem client noch gepuscht werden
  NETCS_ACCEPT_UPDATE,    // so, jetzt will der client jede menge updates haben
  NETCS_UPDATE_IN_PROGRESS, // der client muss den update erst verarbeiten
};

struct net_client {
  enum net_client_status status;
  ssize_t pos;
  int data_fd;
};

struct net_map_info {
  int pipefd[2];      // pipe zur kommunikation papa <-> kind
  pid_t ppid, cpid;   // papa-pid, kind-pid

  int map_fd;         // fd des shm-dinx
  int current_map;    // zeigt auf den gerade zu befuellenden / lesenden map_fd

  int update_fd;
  int current_update;

  struct net_client* nc;  // status-infos der verbundenen clients
};


/*
void net_client_connect(int fd, fd_set* master, int* fdmax, struct timeval* tv);
void net_client_talk(int fd, fd_set* master);
void net_timer(struct timeval* tv);
int net_pipe(int fd, struct timeval* tv);
void net_client_loop(int pipe_fd, int net_fd);
*/
#endif


#endif
