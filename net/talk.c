#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <sys/mman.h>

#include <logging.h>

#include "network.h"

#define PORT "8080"
#define MAX_CONNECTION 100
#define MAX_USERS 100

#if 0
struct info {
  char* start;    // start malloc
  size_t size;    // size von malloc
  size_t unused;  // start + unused zeigen auf die 1. leere zelle
  char last_char; // terminator

  size_t read_next;    // start + read_next zeigen auf den noch nicht gelesenen bereich
};

/* fuegt text mit laenge len in info hinzu
 */
static void append(struct info* info, char* text, int len) {

  // das passt wohl so nicht mehr rein
  if (len + info->unused > info->size) {
    if (info->read_next == 0) {
      // buffer hat keine ungenutzten bereiche mehr -> mehr speicher anfordern
      size_t new_size = len > info->size ? info->size + len : info->size * 2;
      char* tmp = (char*)realloc(info->start, new_size);
      if (tmp == NULL) { perror("realloc"); exit(1); }
      info->start = tmp;
      info->size = new_size;
    } else {
      // buffer hat noch platz -> diesen nutzen
      memmove(info->start, info->start + info->read_next, info->unused - info->read_next);
      info->unused -= info->read_next;
      info->read_next = 0;
    }
  }

  // was passt noch rein
  len = len + info->unused > info->size ? info->size - info->unused : len;

  memcpy(info->start + info->unused, text, len);

  info->unused += len;
}

/* gibt einen pointer data auf die naechste ungelesene zeile 
 * (ohne \n bzw. \r) aus info zur??ck. len spezifiziert die laenge in data
 */
static int read_next_line(struct info* info, char** data, int* len) {

  if (info->read_next == info->unused) {
    printf("  end of buffer (1)\n");
    return -1;
  }

  // komische konstellation wie \n\r als nur einen zeilenumbruch handeln
  if ( ((info->last_char == '\n') && (info->start[info->read_next] == '\r')) ||
      ((info->last_char == '\r') && (info->start[info->read_next] == '\n')) ) {
    info->read_next++;
  }

  // nach \n bzw. \r suchen
  for (size_t i = info->read_next; i < info->unused; ++i) {
    if ( (info->start[i] == '\n') || (info->start[i] == '\r') ) {
      (*len) = i - info->read_next;
      (*data) = info->start + info->read_next;
      info->last_char = info->start[i];
      info->read_next = i + 1;  // es gilt: i < info->unused -> i + 1 ist nie > info->unused, max ==
      return 0;
    }
  }

  // keinen terminator gefunden
  if (info->read_next == info->unused) {
    printf("  end of buffer (2)\n");
    return -1;
  } else if (info->read_next < info->unused) {
    printf("  not terminated\n");
    (*len) = info->unused - info->read_next;
    (*data) = info->start + info->read_next;
    return -2;
  } else {
    printf("  was mach ich hier??\n");
    return -3;
  }
}

static int read_block(struct info* info, char* terminator) {

}


int main() {

  struct info info;

  info.start = (char*)malloc(DEFAULT_SIZE);
  if (info.start == NULL) { perror("malloc"); }
  info.size = DEFAULT_SIZE;
  info.unused = 0;

  info.read_next = 0;

  append(&info, "hallo\n", 6);
  append(&info, "hallo\n", 6);

  append(&info, "foo\r\n", 5);
  append(&info, "\n\r", 2);
  append(&info, "hallo...", 8);
  append(&info, "\r", 1);
  append(&info, "__END__", 7);

  printf("start = >%.*s<\n", (int)info.unused, info.start);

  char* text;
  int len;

  for (int i = 0; i < 10; ++i) {
    if (read_next_line(&info, &text, &len) < 0) {
      printf("nix gefunden...\n");
    } else {
      printf(">%.*s<\n", len, text);
    }
  }

}


#endif

struct userstate {
  char user_name[128];
  int have_auth;
  void (*fkt)(struct userstate*, int);

  char* data_next;  // zeigt auf die kommenden daten
  char* data_start; // zeigt auf die adresse die von mmap reserviert wurde
  size_t data_len;  // groesse von dem von mmap reservierten bereich
  int data_fd;      // fd von mmap
};

static struct userstate* us;


#if 1
static int get_line(struct userstate* us, char** data, int *len) {

  (*data) = us->data_next;
  (*len) = 5;
  return 0;

#if 0
  size_t i = us->data_pos;
  (*len) = 0;

  while (1) {
    if (us->data[i++] == '\n') {
      us->data_pos = i;
      return 0;
    }

    if (i == us->data_len) {
      // kein \n gefunden
      (*len) = 0;
      return -1;
    }

    (*len)++;
  }
#endif
}
#endif

static void _login(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (get_line(us, &data, &len) == -1) {
    log_msg("keine daten...");
    return; // kein return gefunden
  }

  printf("login name = %.*s\n", len, data);
  write(write_fd, "hallo du knackwurst\n", 20);
}

static void _new_account(struct userstate* us, int write_fd) {

}

static void _read(struct userstate* us, int fd, char* in, int len) {
  printf("read = %s\n", in);
}



static void net_client_connect(int fd, fd_set* master, int* fdmax) {
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

  const char tmp_file[] = "wpn_tmp";

  us[newfd].user_name[0] = '\0';
  us[newfd].have_auth = 0;
  us[newfd].fkt = _login;
  us[newfd].data_fd = shm_open(tmp_file, O_CREAT | O_EXCL | O_RDWR, 0700);
  if (shm_unlink(tmp_file) == -1) { log_perror("shm_unlink"); /* das knallt dann spaeter */ }
  if (us[newfd].data_fd == -1) { log_perror("warum zum teufel kann ich wpn_tmp nicht anlegen??"); return; }
  us[newfd].data_len = 1;
  us[newfd].data_start = mmap(0, us[newfd].data_len, PROT_READ | PROT_WRITE, MAP_SHARED, us[newfd].data_fd, 0);
  if (us[newfd].data_start == MAP_FAILED) { log_perror("mmap"); /* TODO: jetzt sollte man was tun */ }
  us[newfd].data_next = us[newfd].data_start;

  const char msg[] = "# Hallo Weltraumreisender,\n"
    "#\n"
    "# wenn Du einen neuen Account haben willst, dann gib keinen Loginnamen an.\n"
    "# (Also jetzt eiinfach nur RETURN druecken.)\n"
    "# Dann hast Du die Moeglichkeit Dir einen neuen Account anzulegen. Ansonsten\n"
    "# gib Deinen Usernamen und Dein Passwort ein\n"
    "100 login: ";

  write(newfd, msg, sizeof(msg));
}


static ssize_t fd_copy(int read_fd, int write_fd) {
  char buffer[512];
  ssize_t len = 0, r, w;

  do {
    r = recv(read_fd, buffer, sizeof(buffer), 0);
    if (r == -1) {
      log_perror("fd_copy");
      return 0;
    }

    len += r;

    if (r > 0) {
      w = write(write_fd, buffer, r);
      if (w != r) {
        if (w == -1) {
          log_perror("fd_copy");
        } else {
          log_msg("fd_copy: anzahl der gelesenen(%d) und der geschriebenen(%d) bytes passen nicht - return 0", r, w);
        }
        return 0;
      }
    }
  
  } while(r == sizeof(buffer));

  return len;
}

static void net_client_talk(int fd, fd_set* master) {

  size_t old_size = us[fd].data_len;
  ssize_t len = fd_copy(fd, us[fd].data_fd);

  if (len == 0) {
    // client mag nicht mehr mit uns reden
    log_msg("<%d> client hang up", fd);

    // sachen aus userstate aufraeumen
    if (munmap(us[fd].data_start, us[fd].data_len) == -1) { log_perror("net_client_talk"); /* TODO noch was? */ }
    close(us[fd].data_fd);

    // sachen globale kommunikation aufraeumen
    close(fd);
    FD_CLR(fd, master);
    return;
  }

  // wir sind nun um len bytes groesser -> memory remappen
  us[fd].data_len += len;
  char* new_buffer = mremap(us[fd].data_start, old_size, us[fd].data_len, MREMAP_MAYMOVE);
  if (new_buffer == MAP_FAILED) { log_perror("net_client_talk"); /* TODO */ }
  us[fd].data_start = new_buffer;

  // nun die eigentliche fkt aufrufen
  us[fd].fkt(&us[fd], fd);
}



int main() {
	log_open("test_talk_main.log");
	log_msg("---------------- new start");

  us = (struct userstate*)malloc(sizeof(struct userstate) * MAX_USERS);
  if (us == NULL) { log_msg("malloc us == NULL: kein platz"); exit(EXIT_FAILURE); }

	int listener = network_bind(PORT, MAX_CONNECTION);
	if (listener == -1) { log_msg("kann auf port "PORT" nicht binden... exit"); exit(EXIT_FAILURE); }

  int net_fd = listener;

  fd_set rfds, rfds_tmp;
  struct timeval tv = {5, 0}; // 5 sec
  int ret;

  FD_ZERO(&rfds);
//  FD_SET(pipe_fd, &rfds);
  FD_SET(net_fd, &rfds);

  int max_fd = net_fd;

  for(;;) {
    rfds_tmp = rfds;
    ret = select(max_fd + 1, &rfds_tmp, NULL, NULL, &tv);

    if (ret == -1) { log_perror("select"); exit(EXIT_FAILURE); }

    if (ret == 0) {
      // timer hat zugeschlagen
//      net_timer(&tv);
      tv.tv_sec = 5;
      tv.tv_usec = 0;
    } else {
      int max_fd_tmp = max_fd;  // brauchen wir, da sich max_fd durch client-connects ??ndern kann
      for (int fd = 0; fd <= max_fd_tmp; ++fd) {
        if (FD_ISSET(fd, &rfds_tmp)) {
//          if (fd == pipe_fd) {
//            ret = net_pipe(fd, &tv);
//            if (ret == -1) { return; } // papa-pipe redet nicht mehr mit uns
//          } else 
          if (fd == net_fd) {
            // client connect
            net_client_connect(net_fd, &rfds, &max_fd);
          } else {
            // client sagt irgendwas...
            net_client_talk(fd, &rfds);
          }
        } // FD_SET
      } // for
    } // else
  } // while


	sleep(100);

}


