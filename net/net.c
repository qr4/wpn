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

#include "pstr.h"

#include "net.h"

#define PORT "8080"
#define MAX_CONNECTION 50

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define NET_MAP "/wpn_map_"
#define NET_MAP_SIZE 4

#define NET_UPDATE "/wpn_update_"
#define NET_UPDATE_SIZE 16

int netpid;

struct {
  int map_pipe;         // pipe papa <-> sohn fuer notify new map
  unsigned int map_id;  // zaehler fuer name der akt. map
  int map_fd;           // vater: fd zum shm der aktuellen map

  int update_pipe;        // pipe papa <-> sohn fuer notify new update
  unsigned int update_id; // zaehler fuer name des akt. updates
  int update_fd;          // vater: fd zum shm des aktuellen updates

  pid_t pid;            // pid vom sohn

  uint64_t relation_counter;  // zaehlt bei jedem hoch um die reihenfolge von map und update zu erkennen
} net = { .map_id = 0, .update_id = 0, .relation_counter = 0 };


struct shm_info {
  int fd;         // fd vom shm-eumel, -1 = wird nicht verwendet
  uint64_t relation;  // wert des relation_counter bei der annahme
  
  // mmap info
  void* addr;     // wo liegt das zeux?
  size_t length;  // groesse
};

static struct shm_info* map_si;
static struct shm_info* update_si;


struct map_client_info {
  int isConnected;                // 0 = nicht konnected, 1 = connected
  
  unsigned int current_map_id;    // id der map die gesendet wird
  size_t send_pos_map;

  unsigned int current_update_id; // id des updates was gesendet wird
  size_t send_pos_update;
};

static struct map_client_info* ci;

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
  close(net.map_fd); // fertig mit der map

  // dem sohn was zum arbyten geben
  unsigned char id[] = { '\0' + net.map_id };
  switch (write(net.map_pipe, id, sizeof(id))) {
    case -1: 
      log_perror("map_flush write");
      exit(EXIT_FAILURE);
    case 0: 
      log_msg("map_flush: blocking write schreibt 0 byte????");
      exit(EXIT_FAILURE);
    case sizeof(id):
      break;  // alles okay
    default:
      log_msg("map_flush hae????");
      exit(EXIT_FAILURE);
  }

  // neue id bauen
  net.map_id = (net.map_id + 1) >= NET_MAP_SIZE ? 0 : net.map_id + 1;

  // neue map aufmachen
  struct pstr file = { .used = 0 };
  pstr_append_printf(&file, NET_MAP"0x%x", net.map_id);

  while( (net.map_fd = shm_open(pstr_as_cstr(&file), O_CREAT | O_EXCL | O_RDWR, 0666)) == -1 ) {
    if (errno == EEXIST) {
      log_msg("map: netio-client kommt nicht hinterher... schlafe 100000usec");
      usleep(100000);
    } else {
      log_perror("shm_open map");
      exit(EXIT_FAILURE);
    }
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
  close(net.update_fd); // fertig mit dem update

  // dem sohn was zum arbyten geben
  unsigned char id[] = { '\0' + net.update_id };
  switch (write(net.update_pipe, id, sizeof(id))) {
    case -1: 
      log_perror("update_flush write");
      exit(EXIT_FAILURE);
    case 0: 
      log_msg("update_flush: blocking write schreibt 0 byte????");
      exit(EXIT_FAILURE);
    case sizeof(id):
      break;  // alles okay
    default:
      log_msg("update_flush: update_flush hae????");
      exit(EXIT_FAILURE);
  }

  // neue id bauen
  net.update_id = (net.update_id + 1) >= NET_UPDATE_SIZE ? 0 : net.update_id + 1;

  // neues update aufmachen
  struct pstr file = { .used = 0 };
  pstr_append_printf(&file, NET_UPDATE"0x%x", net.update_id);

  while( (net.update_fd = shm_open(pstr_as_cstr(&file), O_CREAT | O_EXCL | O_RDWR, 0666)) == -1 ) {
    if (errno == EEXIST) {
      log_msg("update: netio-client kommt nicht hinterher... schlafe 100000usec");
      usleep(100000);
    } else {
      log_perror("shm_open update");
      exit(EXIT_FAILURE);
    }
  }
}

//
// ------------------------------------------------------------------------------------------------
//

//
//
//
static void new_connection(int net_fd, fd_set* rfds, fd_set* wfds, int* max_fd) {

  struct sockaddr_storage remoteaddr;
  socklen_t addrlen = sizeof(remoteaddr);
  int newfd = accept(net_fd, (struct sockaddr *)&remoteaddr, &addrlen);

  if (newfd == -1) {
    log_perror("net accept");
    return;
  }

  if (newfd >= MAX_CONNECTION) {
    log_msg("net: max connections reached. can't handle this connection");
    close(newfd);
    return;
  }
  
  // den client non-blocking machen
  int flags = fcntl(newfd, F_GETFL);
  if (flags == -1) { 
    log_perror("non-blocking (1)");
    close(newfd);
    return;
  }
  if (fcntl(newfd, F_SETFL, flags | O_NONBLOCK) == -1) {
    log_perror("non-blocking (2)");
    close(newfd);
    return;
  }

  ci[newfd].isConnected = 1;    // wir sind verbunden
  
  ci[newfd].current_map_id = 0; // wir brauchen die map
  ci[newfd].send_pos_map = 0;   // haben 0 bytes gelesen

  ci[newfd].current_update_id = 0;
  ci[newfd].send_pos_update = 0;
  
  // 1x lauschen...
  FD_SET(newfd, rfds);  // empfangen von daten... 
  FD_SET(newfd, wfds);  // dem hier dinge schicken (die map!!! und dann updates!!!)

  if (newfd > *max_fd) { *max_fd = newfd; } // update groessten fd

  char remoteIP[INET6_ADDRSTRLEN];
  log_msg("net: <%d> new connection from %s",
    newfd,
    inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN));

  return;
}

//
//
//
static void disconnect_client(int fd, fd_set* rfds, fd_set* wfds, char* msg) {

  if (ci[fd].isConnected == 0) {
    return; // not connected
  }
  
  close(fd);

  FD_CLR(fd, rfds);
  FD_CLR(fd, wfds);

  ci[fd].isConnected = 0;

  log_msg("disconnect_client %d - %s", fd, msg);

  return;
}

//
//
//
static void handle_map(fd_set* rfds, fd_set* wfds) {

  unsigned char id[] = { '\0' };
  switch (read(net.map_pipe, id, sizeof(id))) {
    case -1:
      log_perror("read handle_map");
      exit(EXIT_FAILURE);
    case 0:
      log_msg("???????? handle_map read 0 byte");
      exit(EXIT_FAILURE);
    case 1:
      break;
    default:
      log_msg("handle_map read hea?");
      exit(EXIT_FAILURE);
  }

  net.map_id = id[0];
  map_si[net.map_id].relation = ++net.relation_counter;

  // die clients disconnecten die jetzt immer noch diese map verwenden
  for (int fd = 0; fd < MAX_CONNECTION; ++fd) {
    if (ci[fd].current_map_id == net.map_id) {
      // try to disconnect
      disconnect_client(fd, rfds, wfds, "to slow");
    }
  }

  // wenn der hier schon verwendet wurde, dann dicht machen
  if (map_si[net.map_id].fd >= 0) {
    int ret = munmap(map_si[net.map_id].addr, map_si[net.map_id].length);
    if (ret == -1) { log_perror("handle_map munmap"); exit(EXIT_FAILURE); }

    close(map_si[net.map_id].fd);
  }

  // so, den neuen oeffnen
  struct pstr file = { .used = 0};
  pstr_append_printf(&file, NET_MAP"0x%x", net.map_id);
  map_si[net.map_id].fd = shm_open(pstr_as_cstr(&file), O_RDONLY, 0666);
  if (map_si[net.map_id].fd == -1) { log_perror("handle_map shm_open"); exit(EXIT_FAILURE); }

  // groesse der nachricht holen
  struct stat sb;
  if (fstat(map_si[net.map_id].fd, &sb) == -1) { log_perror("handle_map stat"); exit(EXIT_FAILURE); }
  map_si[net.map_id].length = sb.st_size;

  // nach dem das ding offen ist koennen wir ihn loeschen
  if (shm_unlink(pstr_as_cstr(&file)) == -1) { log_perror("handle_map shm_unlink"); exit(EXIT_FAILURE); }

  // nun darauf nmapen um den inhat zu lesen
  map_si[net.map_id].addr = mmap(NULL, map_si[net.map_id].length, PROT_READ, MAP_PRIVATE, map_si[net.map_id].fd, 0);
  if (map_si[net.map_id].addr == NULL) { log_perror("handle_map mmap"); exit(EXIT_FAILURE); }

  // so, die clients wollen nun die neue map haben -> auf lesewillig schalten
  for (int fd = 0; fd < MAX_CONNECTION; ++fd) {
    if (ci[fd].isConnected == 1) {
      FD_SET(fd, wfds);
    }
  }
}

//
//
//
static void handle_update(fd_set* rfds, fd_set* wfds) {

  unsigned char id[] = { '\0' };
  switch (read(net.update_pipe, id, sizeof(id))) {
    case -1:
      log_perror("read handle_update");
      exit(EXIT_FAILURE);
    case 0:
      log_msg("???????? handle_update read 0 byte");
      exit(EXIT_FAILURE);
    case 1:
      break;
    default:
      log_msg("handle_update read hea?");
      exit(EXIT_FAILURE);
  }

  net.update_id = id[0];
  update_si[net.update_id].relation = ++net.relation_counter;

  // die clients disconnecten die jetzt immer noch diese update verwenden
  for (int fd = 0; fd < MAX_CONNECTION; ++fd) {
    if (ci[fd].current_update_id == net.update_id) {
      // try to disconnect
      disconnect_client(fd, rfds, wfds, "to slow");
    }
  }

  // wenn der hier schon verwendet wurde, dann dicht machen
  if (update_si[net.update_id].fd >= 0) {
    int ret = munmap(update_si[net.update_id].addr, update_si[net.update_id].length);
    if (ret == -1) { log_perror("handle_update munmap"); exit(EXIT_FAILURE); }

    close(update_si[net.update_id].fd);
  }

  // so, den neuen oeffnen
  struct pstr file = { .used = 0};
  pstr_append_printf(&file, NET_UPDATE"0x%x", net.update_id);
  update_si[net.update_id].fd = shm_open(pstr_as_cstr(&file), O_RDONLY, 0666);
  if (update_si[net.update_id].fd == -1) { log_perror("handle_update shm_open"); exit(EXIT_FAILURE); }

  // groesse der nachricht holen
  struct stat sb;
  if (fstat(update_si[net.update_id].fd, &sb) == -1) { log_perror("handle_update stat"); exit(EXIT_FAILURE); }
  update_si[net.update_id].length = sb.st_size;

  // nach dem das ding offen ist koennen wir ihn loeschen
  if (shm_unlink(pstr_as_cstr(&file)) == -1) { log_perror("handle_update shm_unlink"); exit(EXIT_FAILURE); }

  // nun darauf nmapen um den inhat zu lesen
  update_si[net.update_id].addr = 
    mmap(NULL, update_si[net.update_id].length, PROT_READ, MAP_PRIVATE, update_si[net.update_id].fd, 0);
  if (update_si[net.update_id].addr == NULL) { log_perror("handle_update mmap"); exit(EXIT_FAILURE); }

  // so, die clients wollen nun das neue update haben -> auf lesewillig schalten
  for (int fd = 0; fd < MAX_CONNECTION; ++fd) {
    if (ci[fd].isConnected == 1) {
      FD_SET(fd, wfds);
    }
  }
}


//
//
//
static void consume_client_talk(int fd, fd_set* rfds, fd_set* wfds) {

  char buffer[4000];
  ssize_t len = recv(fd, buffer, sizeof(buffer), 0);

  if ((len == 0) || (len == -1)) {
    // client redet nicht mit uns -> disconnect
    disconnect_client(fd, rfds, wfds, "consume_client_talk");
    return;
  }

  return;
}

//
//
//
void net_send(int fd, fd_set* rfds, fd_set* wfds) {

  if (ci[fd].isConnected == 0) {
    return; // wurde wohl gerade zu gemacht...
  }

  {
    // versuchen angefangene map fertig zu verschicken
    if (ci[fd].send_pos_map > 0) {
      ssize_t bytes_left = map_si[ci[fd].current_map_id].length - ci[fd].send_pos_map;
      if (bytes_left > 0) {
        ssize_t out = write(fd, map_si[ci[fd].current_map_id].addr + ci[fd].send_pos_map, bytes_left);
        if (out == -1) { disconnect_client(fd, rfds, wfds, "net_send (net) out == -1"); return; }
        ci[fd].send_pos_map += out;
        return;
      }
    }
  }

  {
    // versuchen angefangene updates fertig zu verschicken
    if (ci[fd].send_pos_update > 0) {
      ssize_t bytes_left = update_si[ci[fd].current_update_id].length - ci[fd].send_pos_update;
      if (bytes_left > 0) {
        ssize_t out = write(fd, update_si[ci[fd].current_update_id].addr + ci[fd].send_pos_update, bytes_left);
        if (out == -1) { disconnect_client(fd, rfds, wfds, "net_send (continue update) out == -1"); return; }
        ci[fd].send_pos_update += out;
        return;
      }
    }
  }

  // wenn wir hier sind, dann wurden map bzw. update komplett verschickt
  // -> ueberpruefen ob es eine neue map gibt -> dann die schicken
  // wenn nicht, naechstes update suchen

  {
    // gibt es eine neuere map? ja -> die schicken
    
    // nach neuster map suchen
    uint64_t latest_relation = 0;
    int latest_map_id = ci[fd].current_map_id;
    for (int i = 0; i < NET_MAP_SIZE; ++i) {
      if ((map_si[i].relation > latest_relation) && (map_si[i].fd >= 0)) {
        latest_relation = map_si[i].relation;
        latest_map_id = i;
      }
    }

    if (latest_map_id != ci[fd].current_map_id) {
      // es gibt eine neuere map...
      ci[fd].current_map_id = latest_map_id;
      log_msg("map_si[latest_map_id].fd = %d", map_si[latest_map_id].fd);
      ssize_t out = write(fd, map_si[latest_map_id].addr, map_si[latest_map_id].length);
      if (out == -1) { disconnect_client(fd, rfds, wfds, "net_send (start update 1) out == -1"); return; }
      ci[fd].send_pos_map = out;
      return;
    }
  }

  {
    // naechstes update suchen
    // das muss von dem relation_counter von der map sein
    uint64_t min_relation = map_si[ ci[fd].current_map_id ].relation;
    if (update_si[ ci[fd].current_update_id ].relation < min_relation) {
      // alte updates ueberspringen
      uint64_t max_relation = min_relation;
      for (int i = 0; i < NET_UPDATE_SIZE; ++i) {
        if ((update_si[i].relation == (min_relation + 1)) && (update_si[i].fd >= 0)) {
          // wir haben das update nach der neuen map gefunden... -> da weiter machen
          ci[fd].current_update_id = i;
          ssize_t out = write(fd, update_si[i].addr, update_si[i].length);
          if (out == -1) { disconnect_client(fd, rfds, wfds, "net_send (start update 2) out == -1"); return; }
          ci[fd].send_pos_update = out;
          return;
        }
        if (max_relation < update_si[i].relation) {
          max_relation = update_si[i].relation;
        }
      }
      if (max_relation == min_relation) {
        // oh, da existieren wohl noch keine updates -> going to sleep
        FD_CLR(fd, wfds);
        return;
      }
    } else {
      // einfach das naechste update finden
      uint64_t next_relation = update_si[ ci[fd].current_update_id ].relation + 1;
      int next_update_id = (ci[fd].current_update_id + 1) >= NET_UPDATE_SIZE ? 0 : ci[fd].current_update_id + 1;

      if (next_relation == update_si[next_update_id].relation) {
        // found :) -> lesen und da weiter machen
        ci[fd].current_update_id = next_update_id;
        ssize_t out = write(fd, update_si[next_update_id].addr, update_si[next_update_id].length);
        if (out == -1) { disconnect_client(fd, rfds, wfds, "net_send (start update (2)) out == -1"); return; }
        ci[fd].send_pos_update = out;
        return;
      } else if (next_relation < update_si[next_update_id].relation) {
        // wir sind zu lahm... und sollten hier eigentlich nie landen
        disconnect_client(fd, rfds, wfds, "net_send - was mach ich hier??");
        return;
      } else {
        // was kleineres gefunden -> warten auf neue sachen
      }
    }
  }

  {
    // warten auf neue dinge und sachen
    FD_CLR(fd, wfds);
  }

}

//
//
//
static void net_loop(int net_fd) {

  fd_set rfds, rfds_tmp;
  fd_set wfds, wfds_tmp;
  struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
  int ret;

  FD_ZERO(&rfds);
  FD_SET(net_fd, &rfds);
  FD_SET(net.map_pipe, &rfds);
  FD_SET(net.update_pipe, &rfds);

  FD_ZERO(&wfds);

  int max_fd = MAX(net_fd, MAX(net.map_pipe, net.update_pipe));

  for(;;) {
    rfds_tmp = rfds;
    wfds_tmp = wfds;
    ret = select(max_fd + 1, &rfds_tmp, &wfds_tmp, NULL, &tv);

    if (ret == -1) { log_perror("net_loop select"); exit(EXIT_FAILURE); }

    if (ret == 0) {
      // dinge mit dem timer machen...
      tv.tv_sec = 1;
    } else {
      int max_fd_tmp = max_fd;

      for (int fd = 0; fd <= max_fd_tmp; ++fd) {

        // jemand hat uns was geschickt...
        if (FD_ISSET(fd, &rfds_tmp)) {
          if (fd == net_fd) {
            // handle new connection
            new_connection(net_fd, &rfds, &wfds, &max_fd);
          } else if (fd == net.map_pipe) {
            // handle new incoming map-data
            handle_map(&rfds, &wfds);
          } else if (fd == net.update_pipe) {
            // handle new incoming update-data
            handle_update(&rfds, &wfds);
          } else {
            // handle unused client-laber-dinge
            consume_client_talk(fd, &rfds, &wfds);
          }
        } // FD_ISSET - read

        // jemand will daten haben...
        if (FD_ISSET(fd, &wfds_tmp)) {
          net_send(fd, &rfds, &wfds);
        } // FD_ISSET - write

      } // for fd
    } // else
  } // for
}

//
//
//
void net_init() {

  // shm-left-overs beseitigen
  for (int i = 0; i <= 0xff; ++i) {
    struct pstr file;

    pstr_clear(&file);
    pstr_append_printf(&file, NET_MAP"0x%x", i);
    if (shm_unlink(pstr_as_cstr(&file)) == 0) {
      log_msg("remove old shm %s", pstr_as_cstr(&file));
    }

    pstr_clear(&file);
    pstr_append_printf(&file, NET_UPDATE"0x%x", i);
    if (shm_unlink(pstr_as_cstr(&file)) == 0) {
      log_msg("remove old shm %s", pstr_as_cstr(&file));
    }
  }

  // communication pipes bauen
  int map_pipe[2], update_pipe[2];

  if (pipe(map_pipe) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }
  if (pipe(update_pipe) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }

  // fork
  net.pid = fork();
  netpid = net.pid;
  if (net.pid == -1) { log_perror("fork"); exit(EXIT_FAILURE); }

  if (net.pid == 0) {
    // wir sind der sohn
    
    // brauchen wir nicht mehr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    net.map_pipe = map_pipe[0];
    close(map_pipe[1]);

    net.update_pipe = update_pipe[0];
    close(update_pipe[1]);

    // speicher fuer shm_info-objekte
    map_si = (struct shm_info*)malloc(sizeof(struct shm_info) * NET_MAP_SIZE);
    if (map_si == NULL) { log_perror("net malloc"); exit(EXIT_FAILURE); }
    for (int i = 0; i < NET_MAP_SIZE; ++i) {
      map_si[i].fd = -1;
    }

    update_si = (struct shm_info*)malloc(sizeof(struct shm_info) * NET_UPDATE_SIZE);
    if (update_si == NULL) { log_perror("net malloc"); exit(EXIT_FAILURE); }
    for (int i = 0; i < NET_UPDATE_SIZE; ++i) {
      update_si[i].fd = -1;
    }
    
    // speicher fuer map_client_info
    ci = (struct map_client_info*)malloc(sizeof(struct map_client_info) * MAX_CONNECTION);
    if (ci == NULL) { log_perror("net malloc"); exit(EXIT_FAILURE); }
    for (int i = 0; i < MAX_CONNECTION; ++i) {
      ci[i].isConnected = 0;
    }

    // bind network
    int net_fd = network_bind(PORT, MAX_CONNECTION);
    if (net_fd == -1) { log_msg("kann auf port "PORT" nicht binden... exit"); exit(EXIT_FAILURE); }

    // the big loop
    net_loop(net_fd);

    // exit
    close(map_pipe[0]);
    close(update_pipe[0]);

    log_msg("net client exit");
    _exit(EXIT_SUCCESS);
  }

  // brauchen wir nicht mehr
  close(map_pipe[0]);
  net.map_pipe = map_pipe[1];

  close(update_pipe[0]);
  net.update_pipe = update_pipe[1];

  // noch die fds aufmachen damit {map_, update_}printf rein schreiben koennen
  struct pstr file = { .used = 0 };
  pstr_append_printf(&file, NET_MAP"0x%x", net.map_id);
  net.map_fd = shm_open(pstr_as_cstr(&file), O_CREAT | O_RDWR, 0666);
  if (net.map_fd == -1) { log_perror("net init shm_open"); exit(EXIT_FAILURE); }

  pstr_clear(&file);
  pstr_append_printf(&file, NET_UPDATE"0x%x", net.update_id);
  net.update_fd = shm_open(pstr_as_cstr(&file), O_CREAT | O_RDWR, 0666);
  if (net.map_fd == -1) { log_perror("net init shm_open"); exit(EXIT_FAILURE); }
}




