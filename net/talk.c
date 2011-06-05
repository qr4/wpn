// 
// code by twist<at>nerd2nerd.org
//

#define _GNU_SOURCE

#include <openssl/sha.h>    // fuer pw hashing

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>       /* For mode constants */
#include <fcntl.h>          /* For O_* constants */


#include <errno.h>          // E* error-zeux

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>         // stat

#include <sys/stat.h>
#include <sys/types.h>      // mkdir

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>          // open

#include <sys/mman.h>

#include <logging.h>

#include "network.h"
#include "pstr.h"

#define PORT "8080"
#define MAX_CONNECTION 100
#define MAX_USERS 100

#define HOME "/opt/wpn/"
#define USER_HOME "/opt/wpn/user"

//
// user-info-status-dinge
//

struct userstate {
  char user_name[128];
  int (*fkt)(struct userstate*, int);

  // zwischenpuffer-dinge (z.B. pw, user-info, ...)
  char* tmp;
  size_t tmp_size;

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


// schaut unter USER_HOME/$name nach ob es ein verzeichnis (oder eine andere datei) gibt 
// 1 = ja, 0 = nein
int have_user(char* name, int len) {

  struct pstr path = { .size = 256, .used = sizeof(USER_HOME), .str = USER_HOME "/" };

  pstr_append(&path, name, len);

  struct stat sb;

  if (stat(path.str, &sb) == -1) {
    if (errno != ENOENT) {
      log_perror("stat");
    }
    return 0;
  }

  if ((sb.st_mode & S_IFMT) != S_IFDIR) {
    log_msg("path %s is not a directory - why??", path.str);
  }

  return 1;
}

// legt einen neuen user unter USER_HOME/$name mit password $pw an
int add_user(char *name, int len_name, char *pw, int len_pw) {

  struct pstr path = { .size = 256, .used = sizeof(USER_HOME), .str = USER_HOME "/" };
  pstr_append(&path, name, len_name);

  if (mkdir(path.str, 0700) == -1) {
    log_perror("mkdir");
    return -1;
  }

  SHA_CTX c;
  unsigned char sha1_pw[SHA_DIGEST_LENGTH];
  SHA1_Init(&c);
  SHA1_Update(&c, pw, len_pw);
  SHA1_Final(sha1_pw, &c);

  pstr_append(&path, "/passwd", 7);
  path.str[path.size] = '\0';

  int fd = open(path.str, O_CREAT | O_EXCL | O_WRONLY, 0700);
  if (fd == -1) {
    log_perror("pw-file open");
    return -1;
  }

  write(fd, sha1_pw, SHA_DIGEST_LENGTH);
  close(fd);

  return 0;
}

//
//
//

int _login(struct userstate* us, int write_fd);
int _new_account_name(struct userstate* us, int write_fd);
int _new_account_pw1(struct userstate* us, int write_fd);
int _new_account_pw2(struct userstate* us, int write_fd);
int _set_user_info(struct userstate* us, int write_fd);

enum state { LOGIN, NEW_ACCOUNT_NAME, NEW_ACCOUNT_PW1, NEW_ACCOUNT_PW2, SET_USER_INFO };

struct automaton {
  const char* msg;
  int (*fkt)(struct userstate*, int);
  enum state state;
} a[] = {
  {
    .msg = "# Hallo Weltraumreisender,\n"
      "#\n"
      "# wenn Du einen neuen Account haben willst, dann gib keinen Loginnamen an.\n"
      "# (Also jetzt eiinfach nur RETURN druecken.)\n"
      "# Dann hast Du die Moeglichkeit Dir einen neuen Account anzulegen. Ansonsten\n"
      "# gib Deinen Usernamen und Dein Passwort ein\n"
      "100 login: ",
    .fkt = _login,
    .state = LOGIN
  },
  {
    .msg = "# Hallo neuer User. Gib doch mal Deinen Loginnamen an\n"
      "# Erlaubt sind die Zeichen [a-zA-Z0-9:-+_]. Mehr nicht. Aetsch! Und 3 bis maximal 12 Zeichen!\n"
      "110 username: ",
    .fkt = _new_account_name,
    .state = NEW_ACCOUNT_NAME
  },
  {
    .msg = "# Okay. Dann verrate mir doch bitte noch dein Passwort.\n"
      "# Laenge muss zwischen 6 und sagen wir mal 42 Zeichen liegen.\n"
      "111 passwort: ",
    .fkt = _new_account_pw1,
    .state = NEW_ACCOUNT_PW1
  },
  {
    .msg = "# Okay. Noch einmal - nur zur Sicherheit.\n"
      "112 passwort: ",
    .fkt = _new_account_pw2,
    .state = NEW_ACCOUNT_PW2
  },
  {
    .msg = "# Hier kannst Du noch Informationen angeben, die Du uns mitteilen moechtest.\n"
      "# Z.B. E-Mail-Adresse, wo sitzt Du damit wir Dich schuetteln koennen wenn Du was boeses\n"
      "# machst, oder einfach nur: GEILES SPIEL WAS IHR DA GEBAUT HABT!! Ueber sowas freuen wir\n"
      "# uns natuerlich auch :-)\n"
      "120 (2x <RETURN> wenn ferig mit der Eingabe) info: ",
    .fkt = _set_user_info,
    .state = SET_USER_INFO
  }
};

static int set_state(enum state s, struct userstate* us, int write_fd) {
  for (int i = 0; i < sizeof(a) / sizeof(struct automaton); ++i) {
    if (a[i].state == s) {
      us->fkt = a[i].fkt;
      int ret = write(write_fd, a[i].msg, strlen(a[i].msg));
      if (ret == -1) {
        log_perror("write to client");
      }
      return ret < 0 ? ret : 0;
    }
  }
  log_msg("was mach ich hier. wieso habe ich keine passende fkt gefunden?? state s = %d", s);
  return -1;
}

//
//
//

int _login(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (read_next_line(us, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len == 0) {
    return set_state(NEW_ACCOUNT_NAME, us, write_fd);
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

int _new_account_name(struct userstate* us, int write_fd) {
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

  if (have_user(data, len)) {
    const char msg[] = "503 Sorry. Aber den User gibts schon. Probiers nochmal\n"
      "110 username: ";
    write(write_fd, msg, sizeof(msg));
    return 0;
  }
 
  memcpy(us->user_name, data, len);
  us->user_name[len + 1] = '\0';

  return set_state(NEW_ACCOUNT_PW1, us, write_fd);
}

int _new_account_pw1(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (read_next_line(us, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len < 6) {
    const char msg[] = "504 Dein Passwort ist zu kurz.\n"
      "111 passwort: ";
    write(write_fd, msg, sizeof(msg));
    return 0;
  }

  memcpy(us->tmp, data, len);
  us->tmp[len] = '\0';

  return set_state(NEW_ACCOUNT_PW2, us, write_fd);
}

int _new_account_pw2(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (read_next_line(us, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if ( (strlen(us->tmp) != len) || (strncmp(us->tmp, data, len)) ) {
    const char msg[] = "505 Die eingegebenen Passwoerter stimmen nicht ueberein. Nochmal!\n";
    write(write_fd, msg, sizeof(msg));
    return set_state(NEW_ACCOUNT_PW1, us, write_fd);
  }

  if (add_user(us->user_name, strlen(us->user_name), data, len) == -1) {
    log_msg("ooops. kann user/pw nicht anlegen");
    return -1;
  }

  return 0;
}

int _set_user_info(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (read_next_line(us, &data, &len) < 0) return 0;  // kein '\n' gefunden


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

  const size_t DEFAULT_DATA_SIZE = 4000;
  free(us[newfd].data_start); // TODO: das hier mal anders regeln - realloc und sowas
  us[newfd].data_start = (char*)malloc(DEFAULT_DATA_SIZE);
  if (us[newfd].data_start == NULL) { log_perror("malloc mag mich nicht :-/"); return; }
  us[newfd].data_size = DEFAULT_DATA_SIZE;
  us[newfd].data_unused = 0;
  us[newfd].data_read_next = 0;

  set_state(LOGIN, &us[newfd], newfd);
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
    us[i].tmp = (char*)malloc(4000);
    if (us[i].tmp == NULL) { log_perror("malloc"); exit(1); }
    us[i].tmp_size = 4000;
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


