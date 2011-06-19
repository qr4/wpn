
#include <unistd.h>

#include <unistd.h>
#include <stdlib.h>

#include <logging.h>

#include "pstr.h"
#include "talk.h"

int main() {
  log_open("test_talk_main.log");
  log_msg("---------------- new start");

  init_talk();

  for (int i = 0; i < 100; ++i) {
    for (int id = 100; id < 105; ++id) {
      struct pstr foo = { .used = 0 };
      pstr_append_printf(&foo, "lalala, id = %d, loop = %d\n", id, i);
      talk_log_lua_msg(id, foo.str, foo.used);
    }
    sleep(1);
  }

  talk_kill();

//  kill(talk.cpid, 9);

  exit(EXIT_SUCCESS);
}



