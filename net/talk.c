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



struct userstate {
  char user_name[128];
  int have_auth;
  void (*fkt)(struct userstate*, int);

  char* data;
  size_t data_len;
  int data_fd;
};

static struct userstate* us;



static void _login(struct userstate* us, int write_fd) {
  printf("login name = %.*s\n", (int)us->data_len, us->data);
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
  us[newfd].data = mmap(0, us[newfd].data_len, PROT_READ | PROT_WRITE, MAP_SHARED, us[newfd].data_fd, 0);
  if (us[newfd].data == MAP_FAILED) { log_perror("mmap"); /* TODO: jetzt sollte man was tun */ }

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
    if (munmap(us[fd].data, us[fd].data_len) == -1) { log_perror("net_client_talk"); /* TODO noch was? */ }
    close(us[fd].data_fd);

    // sachen globale kommunikation aufraeumen
    close(fd);
    FD_CLR(fd, master);
    return;
  }

  // wir sind nun um len bytes groesser -> memory remappen
  us[fd].data_len += len;
  char* new_buffer = mremap(us[fd].data, old_size, us[fd].data_len, MREMAP_MAYMOVE);
  if (new_buffer == MAP_FAILED) { log_perror("net_client_talk"); /* TODO */ }
  us[fd].data = new_buffer;

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


