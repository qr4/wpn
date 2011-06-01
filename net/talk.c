#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <sys/mman.h>

#include <logging.h>

#include "network.h"

#define PORT "8080"
#define MAX_CONNECTION 100
#define MAX_USERS 100


struct userstate {
  char user_name[128];
  int have_auth;
  int (*fkt)(struct userstate*, int);

  // empfangsbuffer-dinge
  char* data_start;   // malloc
  size_t data_size;   // groesse vom malloc
  size_t data_unused; // data_start + data_unused zeigen auf die 1. unbenutzte stelle

  char data_last_char;    // letztes gelesenes zeichen (idr sowas wie '\n' oder '\r')
  size_t data_read_next;  // data_start + data_read_next zeigen auf das 1. noch nicht bearbeitete zeichen
};

static struct userstate* us;


static int read_next_line(struct userstate* us, char** data, int* len) {

  if (us->data_read_next == us->data_unused) {
    return -1;  // am ende des buffers angekommen
  }

  // komische konstellation wie \n\r als nur einen zeilenumbruch handeln
  if ( ((us->data_last_char == '\n') && (us->data_start[us->data_read_next] == '\r')) ||
      ((us->data_last_char == '\r') && (us->data_start[us->data_read_next] == '\n')) ) {
    us->data_read_next++;
  }

  // so, nun nach \n bzw \r suchen
  for (size_t i = us->data_read_next; i < us->data_unused; ++i) {
    if ( (us->data_start[i] == '\n') || (us->data_start[i] == '\r') ) {
      (*len) = i - us->data_read_next;
      (*data) = us->data_start + us->data_read_next;
      us->data_last_char = us->data_start[i];
      us->data_read_next = i + 1; // es gilt: i < unused --> i + 1 ist nie > unused, maximal gleich
      return 0;
    }
  }

  return -1;  // keinen terminator gefunden
}

//
//
//

static char _login_string[] = "# Hallo Weltraumreisender,\n"
  "#\n"
  "# wenn Du einen neuen Account haben willst, dann gib keinen Loginnamen an.\n"
  "# (Also jetzt eiinfach nur RETURN druecken.)\n"
  "# Dann hast Du die Moeglichkeit Dir einen neuen Account anzulegen. Ansonsten\n"
  "# gib Deinen Usernamen und Dein Passwort ein\n"
  "100 login: ";
int _login(struct userstate* us, int write_fd);

static char _new_account_string[] = "# Hallo neuer User. Gib doch mal Deinen Loginnamen an\n"
  "# Erlaubt sind die Zeichen [a-zA-Z0-9:-+_]. Mehr nicht. Aetsch! Und 3 bis maximal 12 Zeichen!\n"
  "110 username: ";
int _new_account(struct userstate* us, int write_fd);


int _login(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (read_next_line(us, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len == 0) {
    write(write_fd, _new_account_string, sizeof(_new_account_string));
    us->fkt = _new_account;
    return 0;
  }

  if (len > 3) { // sizeof(us[write_fd].user_name)) {
    const char msg[] = "500 Dein Loginname ist ein wenig lang geraten. Geh wech!\n";
    write(write_fd, msg, sizeof(msg));
    return -1;
  }

  printf("login name = %.*s\n", len, data);
  write(write_fd, "hallo du knackwurst\n", 20);

  return 0;
}

int _new_account(struct userstate* us, int write_fd) {
  char* data;
  int len;
  
  if (read_next_line(us, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if ((len < 3) || (len > 12)) {
    const char msg[] = "501 Nur 3 bis max. 12 Zeichen. Du Knackwurst. Probiers nochmal\n"
      "110 username: ";
    write(write_fd, msg, sizeof(msg));
    return 0;
  }

  const char allowed_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-+_";
  for (int i = 0; i < len; ++i) {
    int found = 0;
    for (int j = 0; j < sizeof(allowed_chars); ++j) {
      if (data[i] == allowed_chars[j]) {
        found = 1;
      }
    }
    if (found == 0) {
      const char msg[] = "502 Nee Du. Lass mal Deine komischen Zeichen stecken. Probiers nochmal\n"
        "110 username: ";
      write(write_fd, msg, sizeof(msg));
      return 0;
    }
  }

  write(write_fd, "ok\n", 3);

  return 0;
}


//
//
//

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

  const size_t DEFAULT_DATA_SIZE = 4000;
  free(us[newfd].data_start); // TODO: das hier mal anders regeln - realloc und sowas
  us[newfd].data_start = (char*)malloc(DEFAULT_DATA_SIZE);
  if (us[newfd].data_start == NULL) { log_perror("malloc mag mich nicht :-/"); return; }
  us[newfd].data_size = DEFAULT_DATA_SIZE;
  us[newfd].data_unused = 0;
  us[newfd].data_read_next = 0;

  write(newfd, _login_string, sizeof(_login_string));
}

static void net_client_talk(int fd, fd_set* master) {

  size_t free_buffer_size = us[fd].data_size - us[fd].data_unused;

  if (free_buffer_size == 0) {
    if (us[fd].data_read_next == 0) {
      // kein platz mehr -> speicher erweitern
      char* tmp = (char*)realloc(us[fd].data_start, us[fd].data_size * 2);
      if (tmp == NULL) { log_perror("realloc mag nicht :-\\"); exit(1); }
      us[fd].data_start = tmp;
      us[fd].data_size *= 2;
    } else {
      // wir koennen ein wenig platz machen in dem wir 
      // den bereich den wir noch brauchen an den anfang schieben
      memmove(us[fd].data_start, us[fd].data_start + us[fd].data_read_next, us[fd].data_unused - us[fd].data_read_next);
      us[fd].data_unused -= us[fd].data_read_next;
      us[fd].data_read_next = 0;
    }

    // so, nun nochmal die neue freie groesse berechnen
    free_buffer_size = us[fd].data_size - us[fd].data_unused;
  }

  ssize_t len = recv(fd, us[fd].data_start + us[fd].data_unused, free_buffer_size, 0);

  if (len == 0) {
    // client mag nicht mehr mit uns reden
    log_msg("<%d> client hang up", fd);

    // sachen globale kommunikation aufraeumen
    close(fd);
    FD_CLR(fd, master);
    return;
  }

  // wir sind nun um len bytes groesser -> memory remappen
  us[fd].data_unused += len;

  // nun die eigentliche fkt aufrufen
  if (us[fd].fkt(&us[fd], fd) < 0) {
    log_msg("return-state sagt verbindung dichtmachen...");
    close(fd);
    FD_CLR(fd, master);
  }
}



int main() {
	log_open("test_talk_main.log");
	log_msg("---------------- new start");

  us = (struct userstate*)malloc(sizeof(struct userstate) * MAX_USERS);
  if (us == NULL) { log_msg("malloc us == NULL: kein platz"); exit(EXIT_FAILURE); }
  for (int i = 0; i < MAX_USERS; ++i) {
    us[i].data_start = (char*)malloc(10); // TODO: nur wegen free oben - besser machen
  }

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

}


