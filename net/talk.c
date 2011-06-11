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
#define CONFIG_HOME "/opt/wpn/etc"

//
// user-info-status-dinge
//

struct userstate {
  struct pstr user;   // name des users
  unsigned int id;    // id des users

  int (*fkt)(struct userstate*, int);   // funktion die aufgerufen werden soll

  // empfangsbuffer-dinge
  struct dstr net_data;

  // zwischenpuffer-dinge (z.B. pw, user-info, ...)
  struct dstr tmp;
};

static struct userstate* us;

ssize_t net_write(int fd, const void* buf, size_t count) {
  ssize_t ret = write(fd, buf, count);
  if (ret == -1) {
    log_perror("net_write");
  } else if (ret != count) {
    log_msg("oops fd %d hat nicht alle daten bekommen; soll = %d, ist = %d", fd, count, ret);
  }
  return ret;
}


// schaut unter USER_HOME/$name nach ob es ein verzeichnis (oder eine andere datei) gibt 
// 1 = ja, 0 = nein
int have_user(char* name, int len) {
  struct pstr path = { .used = sizeof(USER_HOME), .str = USER_HOME "/" };

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

  struct pstr home = { .used = sizeof(USER_HOME), .str = USER_HOME "/" };
  struct pstr file;
  int ret = 0;

  pstr_append(&home, name, len_name);

  if (mkdir(pstr_as_cstr(&home), 0700) == -1) {
    log_perror("mkdir");
    return -1;
  }
  
  if (ret == 0) {
    // passwd schreiben
    pstr_set(&file, &home);
    pstr_append(&file, "/passwd", 7);

    SHA_CTX c;
    unsigned char sha1_pw[SHA_DIGEST_LENGTH];
    SHA1_Init(&c);
    SHA1_Update(&c, pw, len_pw);
    SHA1_Final(sha1_pw, &c);

    int fd = open(pstr_as_cstr(&file), O_CREAT | O_EXCL | O_WRONLY, 0700);
    if (fd == -1) { log_perror("pw-file open"); return -1; }

    if (write(fd, sha1_pw, SHA_DIGEST_LENGTH) != SHA_DIGEST_LENGTH) {
      log_msg("write schreibt nicht alles von SHA_DIGEST_LENGTH; user: %.*s", len_name, name);
      ret = -1;
    }
    close(fd);
  }

  int next_id = -1;
  if (ret == 0) {
    int fd = open(CONFIG_HOME "/last_id", O_CREAT | O_RDWR, 0700);
    if (fd == -1) { log_perror("last_id"); return -1; }

    char id[16];
    ssize_t count = read(fd, id, sizeof(id));
    if (count == -1) {
      // -1 = boese
      log_perror("read last_id geht nicht");
      ret = -1;
    } else if (count == 0) {
      // datei gibt es wohl noch nicht
      next_id = 100;
    } else {
      errno = 0;
      next_id = strtol(id, (char **) NULL, 10);
      if (errno != 0) {
        log_perror(CONFIG_HOME "/last_id convertieren fehlgeschlagen.");
        ret = -1;
      }
      next_id++;
    }

    if (lseek(fd, 0, SEEK_SET) == -1) { log_perror("lseek"); ret = -1; }
    snprintf(id, sizeof(id), "%d\n", next_id);
    if (write(fd, id, strlen(id)) != strlen(id)) {
      log_msg("update last_id geht nicht. last_id = %d", next_id);
      ret = -1;
    }

    close(fd);
  }

  if (ret == 0) {
    // user id schreiben
    pstr_set(&file, &home);
    pstr_append(&file, "/id", 3);

    int fd = open(pstr_as_cstr(&file), O_CREAT | O_EXCL | O_WRONLY, 0700);
    if (fd == -1) { log_perror("id-file open"); return -1; }

    char id[16];
    snprintf(id, sizeof(id), "%d\n", next_id);

    if (write(fd, id, strlen(id)) != strlen(id)) {
      log_msg("write mag id nicht schreiben. id = %s", id);
      ret = -1;
    }
    close(fd);
  }

  return ret;
}

//
//
//

int _login(struct userstate* us, int write_fd);
int _new_account_name(struct userstate* us, int write_fd);
int _new_account_pw1(struct userstate* us, int write_fd);
int _new_account_pw2(struct userstate* us, int write_fd);
int _set_user_info(struct userstate* us, int write_fd);
int _check_password(struct userstate* us, int write_fd);

enum state { LOGIN, NEW_ACCOUNT_NAME, NEW_ACCOUNT_PW1, NEW_ACCOUNT_PW2, SET_USER_INFO, CHECK_PASSWORD };

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
  },
  {
    .msg = "150 passwort: ",
    .fkt = _check_password,
    .state = CHECK_PASSWORD
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

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len == 0) {
    return set_state(NEW_ACCOUNT_NAME, us, write_fd);
  }

  if (len > 12) {
    const char msg[] = "500 Dein Loginname ist ein wenig lang geraten. Geh wech!\n";
    net_write(write_fd, msg, sizeof(msg));
    return -1;
  }

  pstr_set_cstr(&us->user, data, len);

  return set_state(CHECK_PASSWORD, us, write_fd);;
}

int _new_account_name(struct userstate* us, int write_fd) {
  char* data;
  int len;
  
  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if ((len < 3) || (len > 12)) {
    const char msg[] = "501 Nur 3 bis max. 12 Zeichen. Du Knackwurst. Probiers nochmal\n"
      "110 username: ";
    net_write(write_fd, msg, sizeof(msg));
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
      net_write(write_fd, msg, sizeof(msg));
      return 0;
    }
  }

  if (have_user(data, len)) {
    const char msg[] = "503 Sorry. Aber den User gibts schon. Probiers nochmal\n"
      "110 username: ";
    net_write(write_fd, msg, sizeof(msg));
    return 0;
  }
 
  pstr_set_cstr(&us->user, data, len);

  return set_state(NEW_ACCOUNT_PW1, us, write_fd);
}

int _new_account_pw1(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len < 6) {
    const char msg[] = "504 Dein Passwort ist zu kurz.\n"
      "111 passwort: ";
    net_write(write_fd, msg, sizeof(msg));
    return 0;
  }

  dstr_set(&us->tmp, data, len);

  return set_state(NEW_ACCOUNT_PW2, us, write_fd);
}

int _new_account_pw2(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if ( (dstr_len(&us->tmp) != len) || (strncmp(dstr_as_cstr(&us->tmp), data, len)) ) {
    const char msg[] = "505 Die eingegebenen Passwoerter stimmen nicht ueberein. Nochmal!\n";
    net_write(write_fd, msg, sizeof(msg));
    return set_state(NEW_ACCOUNT_PW1, us, write_fd);
  }

  if (add_user(pstr_as_cstr(&us->user), pstr_len(&us->user), data, len) == -1) {
    log_msg("ooops. kann user/pw nicht anlegen");
    return -1;
  }

  return set_state(SET_USER_INFO, us, write_fd);;
}

int _set_user_info(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  net_write(write_fd, "wurst!", 6);

  return 0;
}

int _check_password(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden
#if 0
  struct pstr path = { .used = sizeof(USER_HOME), .str = USER_HOME "/" };
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

  if (write(fd, sha1_pw, SHA_DIGEST_LENGTH) != SHA_DIGEST_LENGTH) {
    log_msg("write schreibt nicht alles von SHA_DIGEST_LENGTH; user: %.*s", len_name, name);
  }
  close(fd);
#endif
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

  dstr_clear(&us[newfd].net_data);

  set_state(LOGIN, &us[newfd], newfd);
}


static void net_client_talk(int fd, fd_set* master) {

  // TODO irgendwie besser mit pstr verheiraten
  char buffer[4000];
  ssize_t len = recv(fd, buffer, sizeof(buffer), 0);

  if (len == 0) {
    // client mag nicht mehr mit uns reden
    log_msg("<%d> client hang up", fd);

    // sachen globale kommunikation aufraeumen
    close(fd);
    FD_CLR(fd, master);
    return;
  }

  // die empfangenen daten in den puffer schieben
  if (dstr_append(&us[fd].net_data, buffer, len) < 0) { log_perror("dstr_append"); exit(1); }

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
    if (dstr_malloc(&us[i].net_data) < 0) { log_perror("pstr_malloc"); exit(1); }
    if (dstr_malloc(&us[i].tmp) < 0) { log_perror("pstr_malloc"); exit(1); }
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


