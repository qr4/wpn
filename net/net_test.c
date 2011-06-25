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

//  sleep(5);

  printf("now sonding stuff...\n");

  for (int loop = 0; loop < 1000000; ++loop) {

    for (int i = 0; i < 20; ++i) {
      map_printf("############################################################# loop: %d, hello map %d\n", loop, i);
    }
    map_flush();

//    printf("xxx\n");
    usleep(1000);

    for (int i = 0; i < 10; ++i) {
      update_printf("********************************************************** loop: %d, hello update %d\n", loop, i);
      update_printf("***************************************************xxxxxxx loop: %d, hello update %d\n", loop, i);
      update_flush();
    }

  }

  sleep(100);

}
