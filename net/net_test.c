//
// code by twist<at>nerd2nerd.org
//
#include <unistd.h>
#include <logging.h>
#include "net.h"

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
