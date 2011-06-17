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

#define PORT "8090"
#define MAX_CONNECTION 100
#define MAX_USERS 100

#define HOME "/opt/wpn"
#define USER_HOME_BY_NAME HOME"/user/by-name"
#define USER_HOME_BY_ID HOME"/user/by-id"
#define CONFIG_HOME HOME"/etc"
#define CONFIG_LAST_ID CONFIG_HOME"/last_id"

//
// user-info-status-dinge
//

struct userstate {
  struct pstr user;   // name des users
  unsigned int id;    // id des users

  int (*fkt)(struct userstate*, int);   // funktion die aufgerufen werden soll

  struct dstr net_data;   // empfangsbuffer-dinge

  struct dstr tmp;        // zwischenpuffer-dinge (z.B. pw, user-info, ...)
  int count;              // zaehl-variable fuer pw-try, show menu, ...
};

static struct userstate* us;

#if 0
int write_file() {
}

int write_user_file(struct userstate* us, char* filename, int len_filename, struct dstr* data) {
  struct pstr msg = { .used = sizeof(USER_HOME), .str = USER_HOME "/" };
  pstr_append(&msg, &us->user);
  pstr_append_cstr(&msg, filename, len_filename);
  return pstr_write_file(&msg, data, 0);
}
#endif

// schaut unter USER_HOME/$name nach ob es ein verzeichnis (oder eine andere datei) gibt 
// 1 = ja, 0 = nein
int have_user(struct pstr* name) {
  struct pstr path = { .used = sizeof(USER_HOME_BY_ID), .str = USER_HOME_BY_ID "/" };
  pstr_append(&path, name);

  struct stat sb;

  if (stat(pstr_as_cstr(&path), &sb) == -1) {
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

int get_id(struct pstr* file) {
  struct pstr id = { .used = 0 };

  int ret = -1;

  if (pstr_read_file(file, &id) == -1) goto err;

  errno = 0;
  ret = strtol(pstr_as_cstr(&id), (char **) NULL, 10);
  if (errno != 0) goto err;
  
  return ret;

err:
  log_msg("probleme beim lesen von %.*s", pstr_len(file), pstr_as_cstr(file));
  log_perror("get_id");
  return -1;
}

void crypt_passwd(const void* pw, unsigned long pw_len, struct pstr* hash) {
  pstr_clear(hash);
  SHA_CTX c;
  SHA1_Init(&c);
  SHA1_Update(&c, pw, pw_len);
  SHA1_Final((unsigned char *)hash->str, &c);
  hash->used = SHA_DIGEST_LENGTH;
  hash->str[SHA_DIGEST_LENGTH] = '\0';
}                    

// legt einen neuen user unter USER_HOME_BY_ID/$id und USER_HOME_BY_NAME/$name (symlink) 
// mit password $pw an. gibt ihm eine neue id (wird auch zurueckgegeben)
// bei "geht nicht" wird -1 zurueckgegeben
int add_user(char *name, int len_name, const void* pw, unsigned long pw_len, unsigned int* user_id) {

  struct pstr home_by_id = { .used = sizeof(USER_HOME_BY_ID), .str = USER_HOME_BY_ID "/" };
  struct pstr home_by_name = { .used = sizeof(USER_HOME_BY_NAME), .str = USER_HOME_BY_NAME "/" };
  struct pstr file;

  {
    // neue user-id bauen
    struct pstr file_last_id = { .used = sizeof(CONFIG_LAST_ID) - 1, .str = CONFIG_LAST_ID };
    if ((*user_id = get_id(&file_last_id)) == -1) return -1;

    (*user_id)++;

    struct pstr id = { .used = 0 };
    pstr_append_printf(&id, "%d\n", *user_id);

    if (pstr_write_file(&file_last_id, &id, O_WRONLY | O_TRUNC) == -1) {
      log_perror("kann "CONFIG_LAST_ID" nicht schreiben!");
      return -1;
    }
  }
  
  pstr_append_printf(&home_by_id, "%d", *user_id);
  pstr_append_cstr(&home_by_name, name, len_name);

  {
    // verzeichnis und symlink fuer den user anlegen
    if (mkdir(pstr_as_cstr(&home_by_id), 0700) == -1) {
      log_perror("mkdir");
      return -1;
    }
    if (symlink(pstr_as_cstr(&home_by_id), pstr_as_cstr(&home_by_name)) == -1) {
      log_perror("symlink home-by-name");
      return -1;
    }
  }

  {
    // user id schreiben
    pstr_set(&file, &home_by_id);
    pstr_append_cstr(&file, "/id", 3);

    struct pstr id = { .used = 0 };
    pstr_append_printf(&id, "%d\n", *user_id);

    if (pstr_write_file(&file, &id, O_CREAT | O_EXCL | O_WRONLY) == -1) {
      log_perror("write user-id");
      log_msg("write mag id nicht schreiben. id = %s", id);
      return -1;
    }
  }

  {
    // passwd schreiben
    struct pstr hash;
    crypt_passwd(pw, pw_len, &hash);

    pstr_set(&file, &home_by_id);
    pstr_append_cstr(&file, "/passwd", 7);

    if (pstr_write_file(&file, &hash, O_CREAT | O_EXCL | O_WRONLY) == -1) {
      log_perror("pw-file write");
      return -1;
    }
  }

  return 0;
}

int write_user_msg(struct userstate* us) {

  struct pstr msg = { .used = sizeof(USER_HOME_BY_NAME), .str = USER_HOME_BY_NAME "/" };
  pstr_append(&msg, &us->user);
  pstr_append_cstr(&msg, "/msg", 4);

  int fd = open(msg.str, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd == -1) {
    log_perror("open msg");
    return -1;
  }

  ssize_t ret = write(fd, dstr_as_cstr(&us->tmp), dstr_len(&us->tmp));
  if (ret == -1) {
    log_perror("write msg");
    close(fd);
    return -1;
  } else if (ret != dstr_len(&us->tmp)) {
    log_msg("msg von %s konnte nicht komplett geschrieben werden. soll = %d, ist = %d", 
      pstr_as_cstr(&us->user), dstr_len(&us->tmp), ret);
  }

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
int _set_noob(struct userstate* us, int write_fd);

int _check_password(struct userstate* us, int write_fd);
int _menu_main(struct userstate* us, int write_fd);
int _menu_config(struct userstate* us, int write_fd);

enum state { LOGIN, NEW_ACCOUNT_NAME, NEW_ACCOUNT_PW1, NEW_ACCOUNT_PW2, SET_USER_INFO, SET_NOOB,
  CHECK_PASSWORD, MENU_MAIN, MENU_CONFIG };

struct automaton {
  const char* msg;
  const char* prompt;
  const char* const* error;   // evtl. irgend wann mal
//    .error = (const char * const []){"hallo??", "wurst!", NULL},
//    .error = (const char *[]) { "foo", "bar" },
  int (*fkt)(struct userstate*, int);
  enum state state;
} const a[] = {
//
// hinweise zur erstellung von menu-texten:
//
// folgende idee: wenn jemand eine gui drann klatschen moechte - oder das ganze einfach
// per skript steuern will, dann soll ein parsen der ausgaben möglichst einfach sein. daher
// werden texte für den menschen mit einem leerzeichen am anfang "verschönert". 
// eingabeaufforderungen haben eine eindeutige id am anfang. so kann der parser unterscheiden
// ob es für ihn gerade wichtig ist
//
  {
    .msg = "\n"
      " Hallo Weltraumreisender,\n"
      "\n"
      " wenn Du einen neuen Account anlegen willst, dann lass die Eingabe einfach leer.\n"
      " Ansonsten gib Deinen Usernamen ein...\n"
      "\n",
    .prompt = "100 login: ",
    .fkt = _login,
    .state = LOGIN
  },
  {
    .msg = "\n"
      " Hallo neuer User. Gib doch mal Deinen Loginnamen an\n"
      " Erlaubt sind die Zeichen [a-zA-Z0-9:-+_]. Mehr nicht. Aetsch! Und 3 bis maximal 12 Zeichen!\n"
      "\n",
    .prompt = "110 username: ",
    .fkt = _new_account_name,
    .state = NEW_ACCOUNT_NAME
  },
  {
    .msg = "\n"
      " Okay. Dann verrate mir doch bitte noch dein Passwort.\n"
      " Laenge muss zwischen 6 und sagen wir mal 42 Zeichen liegen.\n"
      "\n",
    .prompt = "111 passwort: ",
    .fkt = _new_account_pw1,
    .state = NEW_ACCOUNT_PW1
  },
  {
    .msg = "\n"
      " Okay. Noch einmal - nur zur Sicherheit.\n",
    .prompt = "112 passwort: ",
    .fkt = _new_account_pw2,
    .state = NEW_ACCOUNT_PW2
  },
  {
    .msg = "\n"
      " Hier kannst Du noch Informationen angeben, die Du uns mitteilen moechtest.\n"
      " Z.B. E-Mail-Adresse, wo sitzt Du damit wir Dich schuetteln koennen wenn Du was boeses\n"
      " machst, oder einfach nur: GEILES SPIEL WAS IHR DA GEBAUT HABT!! Ueber sowas freuen wir\n"
      " uns natuerlich auch :-)\n"
      " Eine leere Zeile beendet die Eingabe.\n"
      "\n",
    .prompt = "120 info: ",
    .fkt = _set_user_info,
    .state = SET_USER_INFO
  },
  {
    .msg = "\n"
      " So, eine letzte Frage noch. Der noob, auch so ein komischer Nerd, möchte Deinen\n"
      " LUA-Code verwenden/missbrauchen/andere schlimme Dinge damit machen, um seine\n"
      " Plagiats-Erkennungs-Software zu verbessern. Die komplette entstehungsgeschichte\n"
      " Deines Codes bekommt er aber nur, wenn Du ihm das erlaubst und hier \"JA\"\n"
      " eintippst. Sonst nicht. Also jetzt hier \"JA\" ohne die Gänsefüßchen eintippen!!!1!!1!\n"
      " (sonst kommen die bösen Space-Monster und machen Dich karp00t :P)\n"
      " \n",
    .prompt = "130 noob: ",
    .fkt = _set_noob,
    .state = SET_NOOB
  },
  {
    .msg = "\n",
    .prompt = "150 passwort: ",
    .fkt = _check_password,
    .state = CHECK_PASSWORD
  },
  {
    .msg = "\n"
      " Welcome to the   M A I N - M E N U\n"
      "\n"
      " 1 LUA-Konsole oeffen und Schiffchen per Hand steuern\n"
      " 2 Deinen tollen Schiffchen-Code hochladen\n"
      " 3 LUA-Logging-Konsole oeffnen - watching shit scroll by...\n"
      "\n"
      " Bitte treffen Sie Ihre Wahl\n"
      "\n",
    .prompt = "300 menu: ",
    .fkt = _menu_main,
    .state = MENU_MAIN
  },
  {
    .msg = "\n"
      " The C O N F I G - M E N U\n"
      "\n",
    .prompt = "350 config: ",
    .fkt = _menu_config,
    .state = MENU_CONFIG
  }
};

// verschickt die angegebene msg wenn msg != NULL und msg_len > 0
// verschickt den menu-prompt wenn us != NULL
// return -1 wenns nicht geklappt hat, sonst 0
static ssize_t print_msg_and_prompt(int write_fd, const char* msg, int msg_len, struct userstate* us) {

  // msg senden
  if ((msg != NULL) && (msg_len > 0)) {
    ssize_t ret = write(write_fd, msg, msg_len);
    if (ret == -1) {
      log_perror("print_msg_and_prompt (1)");
      return -1;
    } else if (ret != msg_len) {
      log_msg("oops fd %d hat nicht alle daten bekommen (1); soll = %d, ist = %d", write_fd, msg_len, ret);
    }
  }

  // prompt senden (aber den erstmal rausfinden...)
  if (us != NULL) {
    // bah, prompt vom menu raussuchen
    for (int i = 0; i < sizeof(a) / sizeof(struct automaton); ++i) {
      // ah, in dem menu sind wir also
      if (us->fkt == a[i].fkt) {
        ssize_t len = strlen(a[i].prompt);
        // gibt es ueberhaupt was zum verschicken?
        if (len > 0) {
          ssize_t ret = write(write_fd, a[i].prompt, len);
          if (ret == -1) {
            log_perror("print_msg_and_prompt (2)");
            return -1;
          } else if (ret != len) {
            log_msg("oops fd %d hat nicht alle daten bekommen (2); soll = %d, ist = %d", write_fd, len, ret);
          }
        }
        return 0;
      }
    }
    log_msg("panic?! was passiert hier? wieso habe ich keine passende fkt gefunden?");
    return -1;
  }

  return 0;
}

// setzt den angegebenen menu-state s und zeigt menu-info + prompt an
// return -1 wenns nicht geklappt hat (netzwerk, menu not found, ...), sonst 0
static ssize_t set_state(enum state s, struct userstate* us, int write_fd) {
  for (int i = 0; i < sizeof(a) / sizeof(struct automaton); ++i) {
    if (a[i].state == s) {
      us->fkt = a[i].fkt;
      return print_msg_and_prompt(write_fd, a[i].msg, strlen(a[i].msg), us);
    }
  }
  log_msg("was mach ich hier. wieso habe ich keine passende fkt gefunden?? state s = %d", s);
  return -1;
}

//
//
//

// neue verbindung? das menu!
int _login(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  us->count = 0;

  if (len == 0) {
    return set_state(NEW_ACCOUNT_NAME, us, write_fd);
  }

  if (len > 12) {
    const char msg[] = "500 Dein Loginname ist ein wenig lang geraten. Geh wech!\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return -1;
  }

  pstr_set_cstr(&us->user, data, len);

  return set_state(CHECK_PASSWORD, us, write_fd);;
}

//
int _new_account_name(struct userstate* us, int write_fd) {
  char* data;
  int len;
  
  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if ((len < 3) || (len > 12)) {
    if (++us->count > 3) { goto err; }
    const char msg[] = "501 Nur 3 bis max. 12 Zeichen. Du Knackwurst. Probiers nochmal\n";
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }

  const char allowed_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-+_";
  for (int i = 0; i < len; ++i) {
    int found = 0;
    for (int j = 0; j < sizeof(allowed_chars); ++j) {
      if (data[i] == allowed_chars[j]) {
        found = 1;
      }
    }
    // boese zeichen gefunden
    if (found == 0) {
      if (++us->count > 3) { goto err; }
      const char msg[] = "502 Nee Du. Lass mal Deine komischen Zeichen stecken. Probiers nochmal\n";
      return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
    }
  }

  us->count = 0;
  pstr_set_cstr(&us->user, data, len);

  if (have_user(&us->user)) {
    const char msg[] = "503 Sorry. Aber den User gibts schon. Probiers nochmal\n";
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }

  return set_state(NEW_ACCOUNT_PW1, us, write_fd);

err:
  {
    const char msg[] = "500 Wie oft willst Du da noch was ungültiges eingeben??\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return -1;
  }
}

//
int _new_account_pw1(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len < 6) {
    if (++us->count > 3) { goto err; }
    const char msg[] = "504 Dein Passwort ist zu kurz.\n";
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }

  us->count = 0;
  dstr_set(&us->tmp, data, len);

  return set_state(NEW_ACCOUNT_PW2, us, write_fd);

err:
  {
    const char msg[] = "500 Wie oft willst Du da noch was ungültiges eingeben??\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return -1;
  }
}

//
int _new_account_pw2(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if ( (dstr_len(&us->tmp) != len) || (strncmp(dstr_as_cstr(&us->tmp), data, len)) ) {
    const char msg[] = "505 Die eingegebenen Passwoerter stimmen nicht ueberein. Nochmal!\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return set_state(NEW_ACCOUNT_PW1, us, write_fd);
  }

  if (add_user(pstr_as_cstr(&us->user), pstr_len(&us->user), (const void*)data, (unsigned long)len, &us->id) == -1) {
    log_msg("ooops. kann user/pw nicht anlegen");
    return -1;
  }

  dstr_clear(&us->tmp); // da kommt jetzt die user_info rein
  us->count = 0;

  return set_state(SET_USER_INFO, us, write_fd);;
}

int _set_user_info(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len == 0) {
    if (write_user_msg(us) == -1) return -1;    // irgendwas ist da schief gegangen
    return set_state(SET_NOOB, us, write_fd);
  }

  if (us->count > 3) {
    const char msg[] = "505 Nee nee. Irgendwas machst Du da falsch. User wurde angelegt. Log Dich einfach neu ein.\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return -1;
  }

  if (dstr_len(&us->tmp) > 10000) {
    us->count++;
    const char msg[] = "505 Das jetzt doch ein wenig viel für eine einfache Beschreibung.\n";
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
  }

  dstr_append(&us->tmp, data, len);

  return print_msg_and_prompt(write_fd, NULL, 0, us);
}

int _set_noob(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if ((len == 2) && (strncasecmp(data, "ja", 2) == 0)) {
    struct pstr noob = { .used = sizeof(USER_HOME_BY_NAME), .str = USER_HOME_BY_NAME "/" };
    pstr_append(&noob, &us->user);
    pstr_append_cstr(&noob, "/noob", 5);

    int fd = open(pstr_as_cstr(&noob), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
      log_msg("user %s möchte, dass der noob darf, schreiben geht aber nicht", pstr_as_cstr(&us->user));
    }

    if (write(fd, "JA\n", 3) == -1) {
      log_perror("write ja");
    }

    close(fd);
  }

  return set_state(MENU_MAIN, us, write_fd);
}

//
int _check_password(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  struct pstr path = { .used = 0 };
  pstr_append_printf(&path, USER_HOME_BY_NAME "/%s/passwd", pstr_as_cstr(&us->user));

  struct pstr hash_platte, hash_user;
  if (pstr_read_file(&path, &hash_platte) == -1) {
    log_msg("user %s und/oder path %s gibt es nicht?", pstr_as_cstr(&us->user), pstr_as_cstr(&path));
    log_perror("pw-file kann nicht geoeffnet werden");
    goto err;
  }
  crypt_passwd((const void*)data, (unsigned long)len, &hash_user);

  if (pstr_cmp(&hash_user, &hash_platte) == 0) {
    // login okay
    pstr_clear(&path);
    pstr_append_printf(&path, USER_HOME_BY_NAME"/%s/id", pstr_as_cstr(&us->user));
    if ((us->id = get_id(&path)) == -1) {
      log_perror("user hat keine id??");
      return -1;
    }
  
    log_msg("successful login: id = %d, user = %s", us->id, pstr_as_cstr(&us->user));

    return set_state(MENU_MAIN, us, write_fd);
  }

  // fall thue
err:
  if (++us->count > 3) {
    const char msg[] = "521 Das war 1x zuviel...\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return -1;
  }

  const char msg[] = "520 Dein Passwort passt nicht. Nochmal!\n";
  return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
}

//
int _menu_main(struct userstate* us, int write_fd) {
  char* data;
  int len;

  if (dstr_read_line(&us->net_data, &data, &len) < 0) return 0;  // kein '\n' gefunden

  if (len == 0) goto out;

  const char msg[] = "540 Unbekannter Menu-Punkt\n";
  if ((len != 1) || (data[0] < '1' || data[0] > '3')) {
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }



out:
  if (++us->count > 3) {
    us->count = 0;    // menu nochmal anzeigen
    return set_state(MENU_MAIN, us, write_fd);
  } else {
    return print_msg_and_prompt(write_fd, NULL, 0, us); // nur den prompt anzeigen
  }
}

int _menu_config(struct userstate* us, int write_fd) {


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

  {
    // sicherstellen, dass alle wichtigen verzeichnisse da sind
    char* dirs[] = { HOME, USER_HOME_BY_NAME, USER_HOME_BY_ID, CONFIG_HOME };
    for (int i = 0; i < sizeof(dirs) / sizeof(char*); ++i) {
      struct stat sb;
      if (stat(dirs[i], &sb) == -1) {
        log_msg("mkdir %s", dirs[i]);
        if (mkdir(dirs[i], 0777) == -1) {
          log_perror("mkdir dirs...");
          exit(EXIT_FAILURE);
        }
      }
    }
    // sicherstellen, dass last_id da ist
    struct pstr file_last_id = { .used = sizeof(CONFIG_LAST_ID) - 1, .str = CONFIG_LAST_ID };
    if (get_id(&file_last_id) == -1) exit(EXIT_FAILURE);
  }

  us = (struct userstate*)malloc(sizeof(struct userstate) * MAX_USERS);
  if (us == NULL) { log_msg("malloc us == NULL: kein platz"); exit(EXIT_FAILURE); }
  for (int i = 0; i < MAX_USERS; ++i) {
    if (dstr_malloc(&us[i].net_data) < 0) { log_perror("pstr_malloc"); exit(EXIT_FAILURE); }
    if (dstr_malloc(&us[i].tmp) < 0) { log_perror("pstr_malloc"); exit(EXIT_FAILURE); }
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


