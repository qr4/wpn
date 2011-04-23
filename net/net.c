// 
// code by twist<at>nerd2nerd.org
//

#include <stdarg.h>
#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>
#include <strings.h>
#include <errno.h>

// shm_open
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <arpa/inet.h>

#include <logging.h>

#include "network.h"

#include "net.h"

#define PORT "8080"
#define MAX_CONNECTION 50

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

// achtung: NET_MAP muss mit _a enden, da a hochgezaehlt wird bis z
#define NET_MAP "/wpn_map_a"
// achtung: NET_UPDATE muss mit _A enden, da A hochgezaehlt wird biw Z
#define NET_UPDATE "/wpn_update_A"

struct net_map_info net;

//
// ---------------- public print-fkts
//

int map_printf(const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vdprintf(net.map_fd, fmt, ap);
  va_end(ap);
  return ret;
}

void map_flush() {
  char map[] = NET_MAP;
  char now[] = { 'a' + net.current_map };

  close(net.map_fd);   // fertig mit der map

  write(net.pipefd[1], now, sizeof(now));  //  client benachrichtigen

  net.current_map = (net.current_map + 1) % ('z' - 'a' + 1);  // auf die naechste zeigen
  now[0] = 'a' + net.current_map;
  map[sizeof(map) - 2] = now[0];

  net.map_fd = shm_open(map, O_CREAT | O_EXCL | O_RDWR, 0700);
  if (net.map_fd == -1) {
    if (errno == EEXIST) {
      log_perror("client konsumiert meine maps nicht schnell genug -- ABBRUCH");
    }
    log_perror("shm_open");
    exit(EXIT_FAILURE); 
  }
}

int update_printf(const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vdprintf(net.update_fd, fmt, ap);
  va_end(ap);
  return ret;
}

void update_flush() {
  char update[] = NET_UPDATE;
  char now[] = { 'A' + net.current_update };

  close(net.update_fd);

  write(net.pipefd[1], now, sizeof(now));

  net.current_update = (net.current_update + 1) % ('Z' - 'A' + 1);
  now[0] = 'A' + net.current_update;
  update[sizeof(update) - 2] = now[0];

  net.update_fd = shm_open(update, O_CREAT | O_EXCL | O_RDWR, 0700);
  if (net.update_fd == -1) {
    if (errno == EEXIST) {
      log_perror("client konsumiert meine updates nicht schnell genug -- ABBRUCH");
    }
    log_perror("shm_open"); 
    exit(EXIT_FAILURE); 
  }
}



//
// ---------------------- internes zeux
//

//
// neue verbindung wurde aufgebaut
//
void net_client_connect(int fd, fd_set* master, int* fdmax, struct timeval* tv) {
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen = sizeof(remoteaddr);
  int newfd = accept(fd, (struct sockaddr *)&remoteaddr, &addrlen);

  if (newfd == -1) {
    log_perror("accept");
    return;
  }

  if (newfd >= MAX_CONNECTION) {
    log_msg("max connections reached. can't handle this connection");
    close(newfd);
    return;
  }

  FD_SET(newfd, master);    // add to master set
  if (newfd > *fdmax) {     // keep track of the max
    *fdmax = newfd;
  }
  
  char remoteIP[INET6_ADDRSTRLEN];
  log_msg("<%d> new connection from %s",
    newfd,
    inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN));

  net.nc[newfd].data_fd = dup(net.map_fd);
  if (net.nc[newfd].data_fd == -1) {
    log_perror("dup");
    close(newfd);
    return;
  }

  net.nc[newfd].status = NETCS_NEW_CONNECTED;
  net.nc[newfd].pos = 0;

  // die neue connection gleich mit sachen versorgen lassen
  tv->tv_sec = 0;
  tv->tv_usec = 0;
}

//
// client sagt irgendwas - das interessiert uns aber nicht
// und wird deshalb nur weggeloggt
//
void net_client_talk(int fd, fd_set* master) {
  char buffer[512];
  ssize_t len = recv(fd, buffer, sizeof(buffer), 0);

  if (len == 0) {
    // client mag nicht mehr mit uns reden
    log_msg("<%d> client hang up", fd);
    close(fd);
    FD_CLR(fd, master);

    net.nc[fd].status = NETCS_NOT_CONNECTED;
    close(net.nc[fd].data_fd);
    return;
  }

  log_msg("<%d> client sagt >%.*s<", fd, len, buffer);
}

//
// timeout - verwenden um langsamen clients neue daten reinzudruecken
//
void net_timer(struct timeval* tv) {
  log_msg("timer hat zugeschlagen!!!");
  int haveMoreWork = 0;
  for (int fd = 0; fd < MAX_CONNECTION; ++fd) {
    switch(net.nc[fd].status) {
      case NETCS_NEW_CONNECTED:
      {
        log_msg("-------------------------------------");
        struct stat st;
        int ret = fstat(net.nc[fd].data_fd, &st);
        if (ret == -1) { log_perror("fstat"); break; }

        log_msg("MAP fd = %d, size = %d", net.nc[fd].data_fd, st.st_size);

        char* data = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, net.nc[fd].data_fd, 0);
        if (data == (char*)-1) { log_perror("mmap"); break; }

        ssize_t len = write(fd, data + net.nc[fd].pos, st.st_size - net.nc[fd].pos);
        if (len == -1) {log_perror("write"); break; }

        net.nc[fd].pos += len;

        if (munmap(data, st.st_size) == -1) { log_perror("munmap"); break; }

        if (len == st.st_size) {
          close(net.nc[fd].data_fd);
          net.nc[fd].status = NETCS_ACCEPT_UPDATE;
        } else {
          haveMoreWork = 1;
        }

        break;
      } while(0);
      case NETCS_UPDATE_IN_PROGRESS:
      {
        log_msg("-------------------------------------");
        struct stat st;
        int ret = fstat(net.nc[fd].data_fd, &st);
        if (ret == -1) { log_perror("fstat"); break; }

        log_msg("UPDATE fd = %d, size = %d", net.nc[fd].data_fd, st.st_size);

        char* data = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, net.nc[fd].data_fd, 0);
        if (data == (char*)-1) { log_perror("mmap"); break; }

        ssize_t len = write(fd, data + net.nc[fd].pos, st.st_size - net.nc[fd].pos);
        if (len == -1) {log_perror("write"); break; }

        net.nc[fd].pos += len;

        if (munmap(data, st.st_size) == -1) { log_perror("munmap"); break; }

        if (len == st.st_size) {
          close(net.nc[fd].data_fd);
          net.nc[fd].status = NETCS_ACCEPT_UPDATE;
        } else {
          haveMoreWork = 1;
        }

        break;

      } while(0);
    }
  }

  if (haveMoreWork) {
    // wenn nicht alles verschickt werden konnte, dann probieren wir es in 10ms noch einmal
    tv->tv_sec = 0;
    tv->tv_usec = 1000 * 10;
    log_msg("INFO: slow client");
  } else {
    // alle sind glücklich und haben ihre daten -> in 5 sek wieder hier
    tv->tv_sec = 5;
    tv->tv_usec = 0;
  }

  log_msg("");
}

//
// es stehen neue daten zum verteilen an
//
int net_pipe(int fd, struct timeval* tv) {
  char buffer[128];
  ssize_t len;
  len = read(fd, buffer, sizeof(buffer));
  if (len == -1) { log_perror("read"); return -1; }
  
  if (len == 0) {
    log_msg("keine verbindung zum vater");
    return -1;
  }

  log_msg("neue sachen zum verteilen (%.*s)...", len, buffer);

  char map[] = NET_MAP;
  char update[] = NET_UPDATE;

  int moep_fd = -1;

  for (ssize_t i = 0; i < len; ++i) {
    if ('a' <= buffer[i] && buffer[i] <= 'z') {
      // new map
      char map[] = NET_MAP;
      map[sizeof(map) - 2] = buffer[i];

      int map_fd = shm_open(map, O_RDONLY, 0700);
      if (map_fd == -1) { log_perror("shm_open"); continue; } // TODO sollten wir hier nicht lieber abbrechen?

      // map aus /dev/tmpfs löschen - wir haben ja noch den fd
      if (shm_unlink(map) == -1) { log_perror("shm_unlink"); /* das knallt dann spaeter */ } 

      if (net.map_fd != -1)
        if (close(net.map_fd) == -1) { log_perror("close"); }

      net.map_fd = map_fd;
    } else if ('A' <= buffer[i] && buffer[i] <= 'Z') {
      // new update
      char update[] = NET_UPDATE;
      update[sizeof(update) - 2] = buffer[i];

      int update_fd = shm_open(update, O_RDONLY, 0700);
      if (update_fd == -1) { log_perror("shm_open"); continue; } // TODO sollten wir hier nicht lieber abbrechen?

      // update aus /dev/tmpfs löschen - wir haben ja den fd
      if (shm_unlink(update) == -1) { log_perror("shm_unlink"); /* das knallt dann spaeter */ }

      if (net.update_fd != -1) 
        if (close(net.update_fd) == -1) { log_perror("close"); }

      net.update_fd = update_fd;
      moep_fd = update_fd;
    }
  }

  if (moep_fd != -1) {
    for (int i = 0; i < MAX_CONNECTION; ++i) {
      if (net.nc[i].status == NETCS_ACCEPT_UPDATE) {
        net.nc[i].status = NETCS_UPDATE_IN_PROGRESS;
        net.nc[i].pos = 0;
        net.nc[i].data_fd = dup(moep_fd);

      }

    }

  }

  // daten weitergeben
  tv->tv_sec = 0;
  tv->tv_usec = 0;

  return 0;
}


void net_client_loop(int pipe_fd, int net_fd) {
  fd_set rfds, rfds_tmp;
  struct timeval tv = {5, 0}; // 5 sec
  int ret;
 
  FD_ZERO(&rfds);
  FD_SET(pipe_fd, &rfds);
  FD_SET(net_fd, &rfds);

  int max_fd = MAX(pipe_fd, net_fd);

  for(;;) {
    rfds_tmp = rfds;
    ret = select(max_fd + 1, &rfds_tmp, NULL, NULL, &tv);

    if (ret == -1) { log_perror("select"); exit(EXIT_FAILURE); }

    if (ret == 0) {
      net_timer(&tv);
    } else {
      int max_fd_tmp = max_fd;  // brauchen wir, da sich max_fd durch client-connects ändern kann
      for (int fd = 0; fd <= max_fd_tmp; ++fd) {
        if (FD_ISSET(fd, &rfds_tmp)) {
          if (fd == pipe_fd) {
            ret = net_pipe(fd, &tv);
            if (ret == -1) { return; } // papa-pipe redet nicht mehr mit uns
          } else if (fd == net_fd) {
            // client connect
            net_client_connect(net_fd, &rfds, &max_fd, &tv);
          } else {
            // client sagt irgendwas... wegloggen und gut
            net_client_talk(fd, &rfds);
          }
        } // FD_SET
      } // for
    } // else
  } // while
}


void net_init() {
  if (pipe(net.pipefd) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }

  net.ppid = getpid();
  net.current_map = 0;
  net.current_update = 0;

  net.cpid = fork();
  if (net.cpid == -1) { log_perror("fork"); exit(EXIT_FAILURE); }

  if (net.cpid == 0) {
    // wir sind der client

    // brauchen wir nicht mehr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
//    close(STDERR_FILENO);
    close(net.pipefd[1]);   // lesen reicht

    net.map_fd = -1;
    net.update_fd = -1;

    net.nc = (struct net_client*)malloc(MAX_CONNECTION * sizeof(struct net_client));
    if (net.nc == NULL) { log_msg("malloc net_client == NULL: kein platz"); exit(EXIT_FAILURE); }
    for (int i = 0; i < MAX_CONNECTION; ++i) {
      net.nc[i].status = NETCS_NOT_CONNECTED;
    }

    int listener = network_bind(PORT, MAX_CONNECTION);
    if (listener == -1) { log_msg("kann auf port "PORT" nicht binden... exit"); exit(EXIT_FAILURE); }

   net_client_loop(net.pipefd[0], listener);

    close(net.pipefd[0]);
    log_msg("client exit");
    _exit(EXIT_SUCCESS);
  }

  // wir sind der papa!
  close(net.pipefd[0]); // schreiben reicht!

  // leftovers beseitigen
  char map[] = NET_MAP;
  char update[] = NET_UPDATE;
  char now[] = { '?' };
  for (int i = 0; i < 'z' - 'a' + 1; ++i) {
    now[0] = 'a' + i;
    map[sizeof(map) - 2] = now[0];
    shm_unlink(map);

    now[0] = 'A' + i;
    update[sizeof(update) - 2] = now[0];
    shm_unlink(update);
  }

  now[0] = 'a' + net.current_map;
  map[sizeof(map) - 2] = now[0];
  net.map_fd = shm_open(map, O_CREAT | O_RDWR, 0700);
  if (net.map_fd == -1) { log_perror("shm_open"); exit(EXIT_FAILURE); }

  now[0] = 'A' + net.current_update;
  update[sizeof(update) - 2] = now[0];
  net.update_fd = shm_open(update, O_CREAT | O_RDWR, 0700);
  if (net.update_fd == -1) { log_perror("shm_open"); exit(EXIT_FAILURE); }
}


#if NET_HAVE_MAIN
int main() {

  log_open("test_net_main.log");
  log_msg("---------------- new start");

  net_init();

  for (int loop = 0; loop < 10; ++loop) {

    for (int i = 0; i < 5; ++i) {
      map_printf("loop: %d, hello map %d\n", loop, i);
    }
    map_flush();

    sleep(2);

    for (int i = 0; i < 10; ++i) {
      update_printf("loop: %d, hello update %d\n", loop, i);
      update_flush();
      sleep(1);
    }

  }

  sleep(100);

}
#endif









