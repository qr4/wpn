// 
// code by twist<at>nerd2nerd.org
//

#define _GNU_SOURCE

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

#include <sys/types.h>      // kill
#include <signal.h>


#include <logging.h>

#include "network.h"
#include "pstr.h"

#include "userstuff.h"

#include "config.h"


#define MAX(a, b)  (((a) > (b)) ? (a) : (b))


//
// fuer die kommunikation vater <-> sohn
//

struct talk_info {
  pid_t cpid;     // pid des sohnes

  int pipe_fd[2]; // fd fuer papa <-> sohn

  // sohn
  union {
    struct {
      int id;     // id des users fuer den die nachricht ist
      int len;    // groesse der nachricht
    } head;
    char data[0];
  } lua_log;
  int lua_log_size; // = wie viele bytes vom "header" haben wir schon

  int log_read;   // anzahl der bytes die von der nachrcht schon verschickt wurden
};

static struct talk_info talk = { .lua_log_size = 0 };


//
// user-info-status-dinge
//

struct userstate {
  struct pstr user;   // name des users
  unsigned int id;    // id des users

  int (*fkt)(char* data, int len, struct userstate*, int);   // funktion die aufgerufen werden soll

  struct dstr net_data;   // empfangsbuffer-dinge

  struct dstr tmp;        // zwischenpuffer-dinge (z.B. pw, user-info, ...)
  int count;              // zaehl-variable fuer pw-try, show menu, ...
};

static struct userstate* us;


//
// sachen für den endlichen automaten für die menu-führung
//

int _login(char*, int, struct userstate* us, int write_fd);

int _new_account_name(char*, int, struct userstate* us, int write_fd);
int _new_account_pw1(char*, int, struct userstate* us, int write_fd);
int _new_account_pw2(char*, int, struct userstate* us, int write_fd);
int _set_user_info(char*, int, struct userstate* us, int write_fd);
int _set_noob(char*, int, struct userstate* us, int write_fd);

int _check_password(char*, int, struct userstate* us, int write_fd);
int _menu_main(char*, int, struct userstate* us, int write_fd);

int _lua_console(char*, int, struct userstate* us, int write_fd);
int _lua_get_code(char*, int, struct userstate* us, int write_fd);
int _lua_monitor(char*, int, struct userstate* us, int write_fd);

int _menu_config(char*, int, struct userstate* us, int write_fd);
int _change_password_old(char*, int, struct userstate* us, int write_fd);
int _change_password_new1(char*, int, struct userstate* us, int write_fd);
int _change_password_new2(char*, int, struct userstate* us, int write_fd);
int _change_user_info(char*, int, struct userstate* us, int write_fd);
int _change_noob(char*, int, struct userstate* us, int write_fd);
int _change_ssh_key(char*, int, struct userstate* us, int write_fd);

enum state { 
  LOGIN, 
    NEW_ACCOUNT_NAME, NEW_ACCOUNT_PW1, NEW_ACCOUNT_PW2, SET_USER_INFO, SET_NOOB,
    CHECK_PASSWORD, 
      MENU_MAIN, 
        LUA_CONSOLE, 
        LUA_GET_CODE, 
        LUA_MONITOR, 
        MENU_CONFIG, 
          CHANGE_PASSWORD_OLD, CHANGE_PASSWORD_NEW1, CHANGE_PASSWORD_NEW2, 
          CHANGE_USER_INFO, 
          CHANGE_NOOB, 
          CHANGE_SSH_KEY };

struct automaton {
  const char* msg;
  const char* prompt;
//  const char* const* error;   // evtl. irgend wann mal
//    .error = (const char * const []){"hallo??", "wurst!", NULL},
//    .error = (const char *[]) { "foo", "bar" },
  int (*fkt)(char*, int, struct userstate*, int);
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
      " 3 LUA-Monitor oeffnen - watching shit scroll by...\n"
      "\n"
      " 0 Konfig-Menu\n"
      "\n"
      " Bitte treffen Sie Ihre Wahl\n"
      "\n",
    .prompt = "300 menu: ",
    .fkt = _menu_main,
    .state = MENU_MAIN
  },
  {
    .msg = "\n"
      " Hint: Öffne in einer andren Konsole den LUA-Monitor um weitere Ausgaben zu sehen\n"
      " Gib .quit<RETURN> ein um die LUA-Konsole zu verlassen\n"
      "\n",
    .prompt = "400 lua: ",
    .fkt = _lua_console,
    .state = LUA_CONSOLE
  },
  {
    .msg = "\n"
      " Paste hier Deinen Lua-Code rein.\n"
      " \"-- .quit\" terminiert die Eingabe (ohne die \"\") und aktiviert den neuen Code.\n"
      "\n",
    .prompt = "410 code: ",
    .fkt = _lua_get_code,
    .state = LUA_GET_CODE
  },
  {
    .msg = "\n"
      " Eine beliebige Eingabe mit <RETURN> beendet Monitor-Mode.\n"
      "\n",
    .prompt = "450 monitor: ",
    .fkt = _lua_monitor,
    .state = LUA_MONITOR
  },
  {
    .msg = "\n"
      " The C O N F I G - M E N U\n"
      "\n"
      " 1 Login-Passwort ändern\n"
      " 2 User-Info ändern\n"
      " 3 Noob-Mode ändern (-> Dein Code für Plagiatserkennungssoftware)\n"
      " 4 ssh-key ändern\n"
      "\n"
      " 0 Main-Menu\n"
      "\n",
    .prompt = "350 config: ",
    .fkt = _menu_config,
    .state = MENU_CONFIG
  },
  {
    .msg = "\n"
      " So, 1x Loginpasswort ändern\n"
      "\n",
    .prompt = "100 altes passwort: ",
    .fkt = _change_password_old,
    .state = CHANGE_PASSWORD_OLD
  },
  {
    .msg = "",
    .prompt = "101 passwort: ",
    .fkt = _change_password_new1,
    .state = CHANGE_PASSWORD_NEW1
  },
  {
    .msg = "",
    .prompt = "102 passwort: ",
    .fkt = _change_password_new2,
    .state = CHANGE_PASSWORD_NEW2
  },
  {
    .msg = "\n"
      " User-Info ändern...\n"
      " Eine leere Zeile beendet die Eingabe.\n"
      "\n",
    .prompt = "120 info: ",
    .fkt = _change_user_info,
    .state = CHANGE_USER_INFO
  },
  {
    .msg = "\n"
      " noob\n"
      "\n",
    .prompt = "120 noob: ",
    .fkt = _change_noob,
    .state = CHANGE_NOOB
  },
  {
    .msg = "\n"
      " ssh-key\n"
      "\n",
    .prompt = "125 sshkey: ",
    .fkt = _change_ssh_key,
    .state = CHANGE_SSH_KEY
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
// ------------------------------------------------------------------------------------------------
//

//
//
//
int _login(char* data, int len, struct userstate* us, int write_fd) {
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
//
//
int _new_account_name(char* data, int len, struct userstate* us, int write_fd) {
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
//
//
int _new_account_pw1(char* data, int len, struct userstate* us, int write_fd) {
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
//
//
int _new_account_pw2(char* data, int len, struct userstate* us, int write_fd) {
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

//
//
//
int _set_user_info(char* data, int len, struct userstate* us, int write_fd) {

  if (len == 0) {
    struct pstr path = { .used = 0 };
    pstr_append_printf(&path, USER_HOME_BY_NAME "/%s/msg", pstr_as_cstr(&us->user));

    if (dstr_write_file(&path, &us->tmp, O_WRONLY | O_CREAT | O_TRUNC) == -1) {
      log_perror("write msg");
      return -1;
    }

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
  dstr_append(&us->tmp, "\n", 1);

  return print_msg_and_prompt(write_fd, NULL, 0, us);
}

//
//
//
int _set_noob(char* data, int len, struct userstate* us, int write_fd) {
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
//
//
int _check_password(char* data, int len, struct userstate* us, int write_fd) {
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

  // fall through
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
//
//
int _menu_main(char* data, int len, struct userstate* us, int write_fd) {
  if (len == 0) goto out;

  dstr_clear(&us->tmp);

  const char msg[] = "540 Unbekannter Menu-Punkt\n";
  if (len != 1) {
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }

  switch (data[0]) {
    case '1': return set_state(LUA_CONSOLE, us, write_fd);
    case '2': return set_state(LUA_GET_CODE, us, write_fd);
    case '3': return set_state(LUA_MONITOR, us, write_fd);
    case '0': return set_state(MENU_CONFIG, us, write_fd);
    default: return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }

out:
  if (++us->count > 3) {
    us->count = 0;    // menu nochmal anzeigen
    return set_state(MENU_MAIN, us, write_fd);
  } else {
    return print_msg_and_prompt(write_fd, NULL, 0, us); // nur den prompt anzeigen
  }
}

//
//
//
int _lua_console(char* data, int len, struct userstate* us, int write_fd) {

  if ((len == 5) && (strncasecmp(data, ".quit", 5) == 0)) return set_state(MENU_MAIN, us, write_fd);

  return print_msg_and_prompt(write_fd, NULL, 0, us); // nur den prompt anzeigen
}

//
//
//
int _lua_get_code(char* data, int len, struct userstate* us, int write_fd) {

  if ((len == 8) && (strncasecmp(data, "-- .quit", 8) == 0)) {
    struct pstr revision_id = { .used = 0 };
    pstr_append_printf(&revision_id, USER_HOME_BY_ID"/%d/revision", us->id);

    int current_version = get_id(&revision_id);
    if (current_version == -1) { 
      current_version = 0; // da gabs dann wohl noch keinen code...
    } else {
      current_version++;
    }

    if (set_id(&revision_id, current_version) == -1){
      log_perror("revision");
      return -1;
    }

    struct pstr file = { .used = 0 };
    pstr_append_printf(&file, USER_HOME_BY_ID"/%d/rev_%.6d", us->id, current_version);
    if (dstr_write_file(&file, &us->tmp, O_WRONLY | O_CREAT) == -1) {
      log_perror("write new revision");
      return -1;
    }

    struct pstr link = { .used = 0 };
    pstr_append_printf(&link, USER_HOME_BY_ID"/%d/current", us->id, current_version);
    unlink(pstr_as_cstr(&link));
    
    pstr_clear(&file);
    pstr_append_printf(&file, "rev_%.6d", current_version);
    if (symlink(pstr_as_cstr(&file), pstr_as_cstr(&link)) == -1) {
      log_perror("symlink new revision");
      return -1;
    }

    return set_state(MENU_MAIN, us, write_fd);
  }

  dstr_append(&us->tmp, data, len);
  dstr_append(&us->tmp, "\n", 1);

  return 0; // kein prompt
}

//
//
//
int _lua_monitor(char* data, int len, struct userstate* us, int write_fd) {
  return set_state(MENU_MAIN, us, write_fd);
}

//
//
//
int _menu_config(char* data, int len, struct userstate* us, int write_fd) {
  if (len == 0) return set_state(MENU_MAIN, us, write_fd);

  const char msg[] = "540 Unbekannter Menu-Punkt\n";
  if (len != 1) {
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }

  switch (data[0]) {
    case '1': return set_state(CHANGE_PASSWORD_OLD, us, write_fd);
    case '2': return set_state(CHANGE_USER_INFO, us, write_fd);
    case '3': return set_state(CHANGE_NOOB, us, write_fd);
    case '4': return set_state(CHANGE_SSH_KEY, us, write_fd);
    default: return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }
}

//
//
//
int _change_password_old(char* data, int len, struct userstate* us, int write_fd) {
  return set_state(MENU_CONFIG, us, write_fd);
}

//
//
//
int _change_password_new1(char* data, int len, struct userstate* us, int write_fd) {
  return set_state(MENU_CONFIG, us, write_fd);
}

//
//
//
int _change_password_new2(char* data, int len, struct userstate* us, int write_fd) {
  return set_state(MENU_CONFIG, us, write_fd);
}

//
//
//
int _change_user_info(char* data, int len, struct userstate* us, int write_fd) {
  return set_state(MENU_CONFIG, us, write_fd);
}

//
//
//
int _change_noob(char* data, int len, struct userstate* us, int write_fd) {
  return set_state(MENU_CONFIG, us, write_fd);
}

//
//
//
int _change_ssh_key(char* data, int len, struct userstate* us, int write_fd) {
  return set_state(MENU_CONFIG, us, write_fd);
}

//
// ------------------------------------------------------------------------------------------------
//

static void net_client_connect(int fd, fd_set* master, int* fdmax) {
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen = sizeof(remoteaddr);
  int newfd = accept(fd, (struct sockaddr *)&remoteaddr, &addrlen);

  if (newfd == -1) {
    log_perror("accept");
    return;
  }

  if (newfd >= TALK_MAX_CONNECTION) {
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
    us[fd].id = -1;

    // sachen globale kommunikation aufraeumen
    close(fd);
    FD_CLR(fd, master);
    return;
  }

  // die empfangenen daten in den puffer schieben
  if (dstr_append(&us[fd].net_data, buffer, len) < 0) { log_perror("dstr_append"); exit(1); }

  // nun die eigentliche fkt aufrufen
  
  char* data;
  int data_len;

  while (dstr_read_line(&us[fd].net_data, &data, &data_len) >= 0) {
    if (us[fd].fkt(data, data_len, &us[fd], fd) < 0) {
      log_msg("return-state sagt verbindung dichtmachen. user = %s", pstr_as_cstr(&us[fd].user));
      close(fd);
      FD_CLR(fd, master);
    }
  }
}


static void socket_talk(fd_set* master) {
  if (talk.lua_log_size < sizeof(talk.lua_log.head)) {
    // "header" noch nicht vollstaendig
    ssize_t read_len = read(talk.pipe_fd[0],    // socket read fd
        talk.lua_log.data + talk.lua_log_size,  // position - evtl. append
        sizeof(talk.lua_log.head) - talk.lua_log_size /* nicht ueber id und len hinausschreiben */);
    if (read_len == -1) { log_perror("read from pipe"); exit(EXIT_FAILURE); }
    if (read_len == 0) { log_msg("keine verbindung zum vater (2)"); exit(EXIT_FAILURE); }
    talk.lua_log_size += read_len;
    talk.log_read = 0;
  } else {
    // "header" ist da -> daten an user weiterleiten
    char buffer[4000];
    int buffer_size = sizeof(buffer) < talk.lua_log.head.len - talk.log_read
      ? sizeof(buffer)
      : talk.lua_log.head.len - talk.log_read;

    // daten lesen
    ssize_t read_len = read(talk.pipe_fd[0], buffer, buffer_size);
    if (read_len == -1) { log_perror("read from pipe"); exit(EXIT_FAILURE); }
    if (read_len == 0) { log_msg("keine verbindung zum vater (2)"); exit(EXIT_FAILURE); }
    talk.log_read += read_len;

    // alle offenen konsolen suchen meldung ausgeben
    for (int i = 0; i < TALK_MAX_USERS; ++i) {
      if ((us[i].id == talk.lua_log.head.id) && (us[i].fkt == _lua_monitor)){
        if (write(i, buffer, buffer_size) == -1) {
          // fail -> verbindung dicht machen
          close(i);
          FD_CLR(i, master);
        }
      }
    }

    // alles an den client weiter gegeben
    if (talk.log_read == talk.lua_log.head.len) {
      talk.lua_log_size = 0;
    }
  }
}


static void talk_client_loop(int net_fd) {
  
  // talk_pipe_fd[0] refers to the read end of the pipe. 
  // talk_pipe_fd[1] refers to the write end of the pipe

  fd_set rfds, rfds_tmp;
  int ret;

  FD_ZERO(&rfds);
  FD_SET(talk.pipe_fd[0], &rfds);
  FD_SET(net_fd, &rfds);

  int max_fd = MAX(net_fd, talk.pipe_fd[0]);

  for(;;) {
    rfds_tmp = rfds;
    ret = select(max_fd + 1, &rfds_tmp, NULL, NULL, NULL);

    if (ret == -1) { log_perror("select"); exit(EXIT_FAILURE); }

    int max_fd_tmp = max_fd;  // brauchen wir, da sich max_fd durch client-connects ??ndern kann
    for (int fd = 0; fd <= max_fd_tmp; ++fd) {
      if (FD_ISSET(fd, &rfds_tmp)) {
        if (fd == talk.pipe_fd[0]) {
          socket_talk(&rfds);
        } else if (fd == net_fd) {
          // client connect
          net_client_connect(net_fd, &rfds, &max_fd);
        } else {
          // client sagt irgendwas...
          net_client_talk(fd, &rfds);
        }
      } // FD_SET
    } // for
  } // while

}


void init_talk() {
 
  // sicherstellen, dass alle wichtigen verzeichnisse da sind
  char* dirs[] = { HOME, USER_HOME, USER_HOME_BY_NAME, USER_HOME_BY_ID, CONFIG_HOME };
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

  // sicherstellen, dass last_id da ist, evtl. neu anlegen
  struct pstr file_last_id = { .used = sizeof(CONFIG_LAST_ID) - 1, .str = CONFIG_LAST_ID };
  if (get_id(&file_last_id) == -1) {
    if (errno == ENOENT) {
      log_msg("creating new last_id-file");
      if (set_id(&file_last_id, 99) == -1) {
//      struct pstr default_id = { .used = 3, .str = "99\n" };
//      if (pstr_write_file(&file_last_id, &default_id, O_WRONLY | O_CREAT ) == -1) {
        log_perror("create new last_id");
        exit(EXIT_FAILURE);
      }
    } else {
      log_perror("get_id...");
      exit(EXIT_FAILURE);
    }
  }
 
  // pipe basteln fuer kommunikation
  // talk_pipe_fd[0] refers to the read end of the pipe. 
  // talk_pipe_fd[1] refers to the write end of the pipe
  if (pipe(talk.pipe_fd) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }

  // talk-client abspalten
  int cpid = fork();
  if (cpid == -1) { log_perror("fork"); exit(EXIT_FAILURE); }

  if (cpid == 0) {
    // wir sind der client
    
    // brauchen wir nicht mehr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // speicher holen
    us = (struct userstate*)malloc(sizeof(struct userstate) * TALK_MAX_USERS);
    if (us == NULL) { log_msg("malloc us == NULL: kein platz"); exit(EXIT_FAILURE); }
    for (int i = 0; i < TALK_MAX_USERS; ++i) {
      us[i].id = -1;
      if (dstr_malloc(&us[i].net_data) < 0) { log_perror("pstr_malloc"); exit(EXIT_FAILURE); }
      if (dstr_malloc(&us[i].tmp) < 0) { log_perror("pstr_malloc"); exit(EXIT_FAILURE); }
    }

    // listening socket aufmachen
	  int net_fd = network_bind(TALK_PORT, TALK_MAX_CONNECTION);
  	if (net_fd == -1) { log_msg("kann auf port "TALK_PORT" nicht binden... exit"); exit(EXIT_FAILURE); }

    talk_client_loop(net_fd);

    close(talk.pipe_fd[0]);
    close(talk.pipe_fd[1]);
    log_msg("talk client exit");

    _exit(EXIT_SUCCESS);
  }

  // wir sind der papa

}


void talk_log_lua_msg(unsigned int user_id, char* msg, int msg_len) {
  write(talk.pipe_fd[1], &user_id, sizeof(unsigned int));
  write(talk.pipe_fd[1], &msg_len, sizeof(int));
  write(talk.pipe_fd[1], msg, msg_len);
}

int talk_get_user_change_code_fd() {
  return talk.pipe_fd[0];
}


// do not use!
void talk_kill() {
  kill(talk.cpid, 9);
}



