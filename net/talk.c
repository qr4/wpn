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
#include "dispatch.h"
#include "config.h"


#define MAX(a, b)  (((a) > (b)) ? (a) : (b))


int talkpid = -1;

//
// fuer die kommunikation vater <-> sohn
//

struct talk_info {
  pid_t cpid;     // pid des sohnes

  // menupunkt: 1 LUA-Konsole oeffen und Heimatwelt per Hand steuern
  int user_code_pipe;         // schickt code vom user an den vater via pipe_com
  int user_code_reply_pipe;   // antwort des interpreters auf den code via pipe_com

  // menupunkt: 2 Deinen tollen Schiffchen-Code hochladen
  int user_change_code_pipe;  // sohn informiert vater ueber neu hochgeladenen code
                              // inhalt: unsigned int mit user-id

  // menupunkt: 3 LUA-Monitor oeffnen - watching shit scroll by...
  int log_lua_msg_pipe;       // vater informiert sohn uber logausgaben eines clients via pipe_com

  //
  // sohn
  //
  struct pipe_com lua_console;  // menupunkt 1
  struct pipe_com lua_log;      // menupunkt 3
};

static struct talk_info talk = { .lua_log.read_head = 0 };


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
int _change_ssh_key(char*, int, struct userstate* us, int write_fd);

enum state { 
  LOGIN, 
    NEW_ACCOUNT_NAME, NEW_ACCOUNT_PW1, NEW_ACCOUNT_PW2, SET_USER_INFO,
    CHECK_PASSWORD, 
      MENU_MAIN, 
        LUA_CONSOLE, 
        LUA_GET_CODE, 
        LUA_MONITOR, 
        MENU_CONFIG, 
          CHANGE_PASSWORD_OLD, CHANGE_PASSWORD_NEW1, CHANGE_PASSWORD_NEW2, 
          CHANGE_USER_INFO, 
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
      " wenn Du einen neuen Account anlegen willst, dann lass die Eingabe einfach\n"
      " leer. Ansonsten gib Deinen Usernamen ein...\n"
      "\n",
    .prompt = "100 login: ",
    .fkt = _login,
    .state = LOGIN
  },
  {
    .msg = "\n"
      " Hallo neuer User. Gib doch mal Deinen Loginnamen an\n"
      " Erlaubt sind die Zeichen [a-zA-Z0-9:-+_]. Mehr nicht. Aetsch!\n"
      " Und 3 bis maximal 12 Zeichen!\n"
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
      " Okay. Noch einmal dein Passwort - nur zur Sicherheit.\n",
    .prompt = "112 passwort: ",
    .fkt = _new_account_pw2,
    .state = NEW_ACCOUNT_PW2
  },
  {
    .msg = "\n"
      " Hier kannst Du noch Informationen angeben, die Du uns mitteilen moechtest.\n"
      " Z.B. eine E-Mail-Adresse, unter der wir Dich erreichen koennen, oder wo auf\n"
      " der GPN Du sitzt damit wir Dich schuetteln koennen wenn Du was boeses machst.\n"
      " Auch ueber Rueckmeldung wie GEILES SPIEL WAS IHR DA GEBAUT HABT!! freuen wir\n"
      " uns natuerlich :-)\n"
      " Eine leere Zeile beendet die Eingabe.\n"
      "\n",
    .prompt = "120 info: ",
    .fkt = _set_user_info,
    .state = SET_USER_INFO
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
      " 1 LUA-Konsole oeffen und Heimatwelt per Hand steuern\n"
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
      " Hint: Oeffne in einer andren Konsole den LUA-Monitor um weitere\n"
      " Ausgaben zu sehen. Gib .quit<RETURN> ein um die LUA-Konsole zu verlassen\n"
      "\n",
    .prompt = "400 lua: ",
    .fkt = _lua_console,
    .state = LUA_CONSOLE
  },
  {
    .msg = "\n"
      " Paste hier Deinen Lua-Code rein.\n"
      " \"-- .quit\" terminiert die Eingabe (ohne die \"\") und aktiviert\n"
      " den neuen Code.\n"
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
      " 1 Login-Passwort aendern\n"
      " 2 User-Info aendern\n"
      "\n"
      " 0 Main-Menu\n"
      "\n",
    .prompt = "350 config: ",
    .fkt = _menu_config,
    .state = MENU_CONFIG
  },
  {
    .msg = "\n"
      " So, 1x Loginpasswort aendern...\n"
      "\n",
    .prompt = "100 altes passwort: ",
    .fkt = _change_password_old,
    .state = CHANGE_PASSWORD_OLD
  },
  {
    .msg = "\n"
      " Und nun das neue Passwort... Remember: mindestens 6 Zeichen.\n"
      "\n",
    .prompt = "101 neues passwort: ",
    .fkt = _change_password_new1,
    .state = CHANGE_PASSWORD_NEW1
  },
  {
    .msg = "\n"
      " und noch 1x zur Sicherheit...\n"
      "\n",
    .prompt = "102 passwort: ",
    .fkt = _change_password_new2,
    .state = CHANGE_PASSWORD_NEW2
  },
  {
    .msg = "\n"
      " User-Info aendern...\n"
      " <RETURN> um nix zu aendern.\n"
      " Eine leere Zeile beendet die Eingabe.\n"
      "\n",
    .prompt = "120 info: ",
    .fkt = _change_user_info,
    .state = CHANGE_USER_INFO
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
    const char msg[] = "500 Wie oft willst Du da noch was ungueltiges eingeben??\n";
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
    const char msg[] = "500 Wie oft willst Du da noch was ungueltiges eingeben??\n";
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

    return set_state(MENU_MAIN, us, write_fd);
  }

  if (us->count > 3) {
    const char msg[] = "505 Nee nee. Irgendwas machst Du da falsch. User wurde angelegt. Log Dich einfach neu ein.\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return -1;
  }

  if (dstr_len(&us->tmp) > 10000) {
    us->count++;
    const char msg[] = "505 Das jetzt doch ein wenig viel fuer eine einfache Beschreibung.\n";
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
  }

  dstr_append(&us->tmp, data, len);
  dstr_append(&us->tmp, "\n", 1);

  return print_msg_and_prompt(write_fd, NULL, 0, us);
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

  if ((len == 5) && (strncasecmp(data, ".quit", 5) == 0)) {
    dstr_clear(&us->tmp);
    return set_state(MENU_MAIN, us, write_fd);
  }

  if (len > 0) {
    if (data[len - 1] == '\\') {
      // wir haben ein \ am ende -> mehr-zeilen-cmd
      dstr_append(&us->tmp, data, len - 1);
      dstr_append(&us->tmp, "\n", 1);
      // return mehrzeilen-prompt
      return print_msg_and_prompt(write_fd, "       > ", 9, NULL);
    } else {
      if (dstr_len(&us->tmp) > 0) {
        // mehrzeilen-cmd, letzte zeile ohne "\"
        dstr_append(&us->tmp, data, len);
        len = dstr_len(&us->tmp);
        data = dstr_as_cstr(&us->tmp);
      }
    }
  } else if (len == 0) {
    if (dstr_len(&us->tmp) > 0) {
      // im alten puffer steht noch was -> ausfuehren
      len = dstr_len(&us->tmp);
      data = dstr_as_cstr(&us->tmp);
    } else {
      // len = 0 und kein puffer -> prompt anzeigen
      return print_msg_and_prompt(write_fd, NULL, 0, us); // nur den prompt anzeigen
    }
  }

  // send command
  int ret =  send_dispatch_msg(talk.user_code_pipe, us->id, data, len);
  dstr_clear(&us->tmp);

  return ret; // kein prompt... wird generiert bei der lua-ruckmeldung
}

//
//
//
int _lua_get_code(char* data, int len, struct userstate* us, int write_fd) {

  if (((len == 7) && (strncasecmp(data, "--.quit", 7) == 0)) ||
    ((len == 8) && (strncasecmp(data, "-- .quit", 8) == 0))) {
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

    int ret = write(talk.user_change_code_pipe, &us->id, sizeof(unsigned int));
    if (ret == -1) { return -1; }

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

  dstr_clear(&us->tmp);
  us->count = 0;

  switch (data[0]) {
    case '1': return set_state(CHANGE_PASSWORD_OLD, us, write_fd);
    case '2': {
                // header
                const char msg[] = "\n"
                  " Aktueller Text:\n"
                  "\n";
                if (print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL) == -1) { return -1; }

                // aktuellen text anzeigen
                struct pstr file = { .used = 0 };
                pstr_append_printf(&file, USER_HOME_BY_ID"/%d/msg", us->id);
                if (dstr_read_file(&file, &us->tmp) == -1) { 
                  log_msg("kann %s nicht lesen", pstr_as_cstr(&file));
                  log_perror("read msg"); 
                  const char msg[] = "-- TEXT NICHT GEFUNDEN --\n";
                  dstr_set(&us->tmp, msg, sizeof(msg));
                }
                if (print_msg_and_prompt(write_fd, dstr_as_cstr(&us->tmp), dstr_len(&us->tmp), NULL) == -1) { return -1; }
                dstr_clear(&us->tmp);
                us->count = 0;
                return set_state(CHANGE_USER_INFO, us, write_fd);
              }
//    case '4': return set_state(CHANGE_SSH_KEY, us, write_fd);
    case '0': return set_state(MENU_MAIN, us, write_fd);
    default: return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }
}

//
//
//
int _change_password_old(char* data, int len, struct userstate* us, int write_fd) {
  struct pstr path = { .used = 0 };
  pstr_append_printf(&path, USER_HOME_BY_ID "/%d/passwd", us->id);

  struct pstr hash_platte, hash_user;
  if (pstr_read_file(&path, &hash_platte) == -1) {
    log_msg("_change_password_old: user %d nicht gefunden, path = %s", us->id, pstr_as_cstr(&path));
    log_perror("pw-file kann nicht geoeffnet werden");
    return -1;
  }
  crypt_passwd((const void*)data, (unsigned long)len, &hash_user);

  if (pstr_cmp(&hash_user, &hash_platte) == 0) {
    // passwort okay
    dstr_clear(&us->tmp);
    us->count = 0;
    return set_state(CHANGE_PASSWORD_NEW1, us, write_fd);
  }

  const char msg[] = "550 Passowrt falsch\n";
  print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);

  return set_state(MENU_CONFIG, us, write_fd);
}

//
//
//
int _change_password_new1(char* data, int len, struct userstate* us, int write_fd) {
  if (len < 6) {
    if (++us->count > 3) { goto err; }
    const char msg[] = "504 Dein Passwort ist zu kurz.\n";
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), us);
  }

  us->count = 0;
  dstr_set(&us->tmp, data, len);

  return set_state(CHANGE_PASSWORD_NEW2, us, write_fd);

err:
  {
    const char msg[] = "500 Wie oft willst Du da noch was ungueltiges eingeben??\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return set_state(MENU_CONFIG, us, write_fd);
  }
}

//
//
//
int _change_password_new2(char* data, int len, struct userstate* us, int write_fd) {
  if ( (dstr_len(&us->tmp) != len) || (strncmp(dstr_as_cstr(&us->tmp), data, len)) ) {
    const char msg[] = "505 Die eingegebenen Passwoerter stimmen nicht ueberein. Nochmal!\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return set_state(CHANGE_PASSWORD_NEW1, us, write_fd);
  }

  struct pstr hash;
  crypt_passwd(data, len, &hash);

  struct pstr file = { .used = 0 };
  pstr_append_printf(&file, USER_HOME_BY_ID "/%d/passwd", us->id);
  if (pstr_write_file(&file, &hash, O_WRONLY | O_TRUNC) == -1) {
    log_msg("probleme beim schreiben von %s", pstr_as_cstr(&file));
    log_perror("_change_password_new2");
    return -1;
  }

  return set_state(MENU_CONFIG, us, write_fd);
}

//
//
//
int _change_user_info(char* data, int len, struct userstate* us, int write_fd) {

  if (len == 0) {
    // user fertig mit eingabe
    if (dstr_len(&us->tmp) > 0) {
      // dinge schreiben
      struct pstr file = { .used = 0 };
      pstr_append_printf(&file, USER_HOME_BY_ID"/%d/msg", us->id);
      log_msg("schreibe text = %s", dstr_as_cstr(&us->tmp));
      if (dstr_write_file(&file, &us->tmp, O_WRONLY | O_TRUNC | O_CREAT) == -1) {
        log_msg("probleme beim schreiben von %s", pstr_as_cstr(&file));
        log_perror("_change_user_info");
        return -1;
      }
    }
    return set_state(MENU_CONFIG, us, write_fd);
  }

  if (us->count > 3) {
    const char msg[] = "505 Nee nee. Irgendwas machst Du da falsch...\n";
    print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
    return -1;
  }


  if (dstr_len(&us->tmp) > 10000) {
    us->count++;
    const char msg[] = "505 Das jetzt doch ein wenig viel fuer eine einfache Beschreibung.\n";
    return print_msg_and_prompt(write_fd, msg, sizeof(msg), NULL);
  }

  dstr_append(&us->tmp, data, len);
  dstr_append(&us->tmp, "\n", 1);

  return print_msg_and_prompt(write_fd, NULL, 0, us);
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

  if (len == 0 || len == -1) {
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

static void dispatch_code_reply(fd_set* master) {
  char* buffer;
  int buffer_len;
  if (dispatch(&talk.lua_console, talk.user_code_reply_pipe, &buffer, &buffer_len) == -1) { exit(EXIT_FAILURE); }

  if (buffer_len > 0) {
    for (int i = 0; i < TALK_MAX_USERS; ++i) {
      if ((us[i].id == talk.lua_console.header.head.id) && (us[i].fkt == _lua_console)) {
        if (write(i, buffer, buffer_len) == -1) {
          // fail -> verbindung dicht machen
          close(i);
          FD_CLR(i, master);
        }
      }
    }
  }
}

static void dispatch_lua_msg(fd_set* master) {
  char* buffer;
  int buffer_len;
  if (dispatch(&talk.lua_log, talk.log_lua_msg_pipe, &buffer, &buffer_len) == -1) { exit(EXIT_FAILURE); }

  if (buffer_len > 0) {
    for (int i = 0; i < TALK_MAX_USERS; ++i) {
      if ((us[i].id == talk.lua_log.header.head.id) && (us[i].fkt == _lua_monitor)) {
        if (write(i, buffer, buffer_len) == -1) {
          // fail -> verbindung dicht machen
          close(i);
          FD_CLR(i, master);
        }
      }
    }
  }
}


static void talk_client_loop(int net_fd) {
  
  fd_set rfds, rfds_tmp;
  int ret;

  FD_ZERO(&rfds);
  FD_SET(talk.user_code_reply_pipe, &rfds);
  FD_SET(talk.log_lua_msg_pipe, &rfds);
  FD_SET(net_fd, &rfds);

  int max_fd = MAX(talk.user_code_reply_pipe, MAX(talk.log_lua_msg_pipe, net_fd));

  for(;;) {
    rfds_tmp = rfds;
    ret = select(max_fd + 1, &rfds_tmp, NULL, NULL, NULL);

    if (ret == -1) { log_perror("talk_client_loop select"); exit(EXIT_FAILURE); }

    int max_fd_tmp = max_fd;  // brauchen wir, da sich max_fd durch client-connects ??ndern kann
    for (int fd = 0; fd <= max_fd_tmp; ++fd) {
      if (FD_ISSET(fd, &rfds_tmp)) {
        if (fd == talk.user_code_reply_pipe) {
          // pipe papa schickt lua-consolen-ausgaben
          dispatch_code_reply(&rfds);
        } else if (fd == talk.log_lua_msg_pipe) {
          // pipe papa schickt lua msgs
          dispatch_lua_msg(&rfds);
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
        log_perror("create new last_id");
        exit(EXIT_FAILURE);
      }
    } else {
      log_perror("get_id...");
      exit(EXIT_FAILURE);
    }
  }
 
  // pipes basteln fuer kommunikation
  int user_code_pipe[2], user_code_reply_pipe[2], user_change_code_pipe[2], log_lua_msg_pipe[2];
  // pipe[0] refers to the read end of the pipe. 
  // pipe[1] refers to the write end of the pipe
  if (pipe(user_code_pipe) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }
  if (pipe(user_code_reply_pipe) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }
  if (pipe(user_change_code_pipe) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }
  if (pipe(log_lua_msg_pipe) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }

  // talk-client abspalten
  int cpid = fork();
  if (cpid == -1) { log_perror("fork"); exit(EXIT_FAILURE); }

  if (cpid == 0) {
    // wir sind der client
    close(user_code_pipe[0]);
    talk.user_code_pipe = user_code_pipe[1];

    talk.user_code_reply_pipe = user_code_reply_pipe[0];
    close(user_code_reply_pipe[1]);

    close(user_change_code_pipe[0]);
    talk.user_change_code_pipe = user_change_code_pipe[1];

    talk.log_lua_msg_pipe = log_lua_msg_pipe[0];
    close(log_lua_msg_pipe[1]);

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

    close(talk.user_code_pipe);
    close(talk.user_code_reply_pipe);
    close(talk.user_change_code_pipe);
    close(talk.log_lua_msg_pipe);
    log_msg("talk client exit");

    _exit(EXIT_SUCCESS);
  }

  // wir sind der papa
  talkpid = cpid;

  talk.user_code_pipe = user_code_pipe[0];
  close(user_code_pipe[1]);

  close(user_code_reply_pipe[0]);
  talk.user_code_reply_pipe = user_code_reply_pipe[1];

  talk.user_change_code_pipe = user_change_code_pipe[0];
  close(user_change_code_pipe[1]);

  close(log_lua_msg_pipe[0]);
  talk.log_lua_msg_pipe = log_lua_msg_pipe[1];
}


// menupunkt 1
int talk_get_user_code_fd() {
  return talk.user_code_pipe;
}

int talk_set_user_code_reply_msg(unsigned int user_id, const char* msg, int msg_len) {
  return send_dispatch_msg(talk.user_code_reply_pipe, user_id, msg, msg_len);
} 

// menupunkt 2
int talk_get_user_code_upload_fd() {
  return talk.user_change_code_pipe;
}

// menupunkt 3
int talk_log_lua_msg(unsigned int user_id, const char* msg, int msg_len) {
  return send_dispatch_msg(talk.log_lua_msg_pipe, user_id, msg, msg_len);
}



// do not use!
void talk_kill() {
  kill(talk.cpid, 9);
}



