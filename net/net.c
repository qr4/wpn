


#include <stdarg.h>
#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>
#include <strings.h>
#include <errno.h>


// shm_open
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

// 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <arpa/inet.h>

#include "../logging/logging.h"

#include "net.h"

#define PORT "8080"
#define MAX_CONNECTION 50

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


struct net_map_info net;

//
// ---------------- public print-fkts
//

int map_printf(const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret = vdprintf(net.map_fd[net.current_map], fmt, ap);
  va_end(ap);
  return ret;
}

void map_flush() {
  close(net.map_fd[net.current_map]);
  net.current_map = 1 - net.current_map;
  
  int fd = shm_open(net.current_map == 0 ? "/wpn_map0" : "/wpn_map1", O_CREAT | O_RDWR, 0700);
  if (fd == -1) { log_perror("shm_open"); exit(EXIT_FAILURE); }

  net.map_fd[net.current_map] = fd;

  write(net.pipefd[1], net.current_map == 0 ? "a" : "b", 1);
}

int update_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void updated_flush() {}



//
// ---------------------- internes zeux
//

int net_bind(char* port) {
  int listener;     // listening socket descriptor
  int yes = 1;      // for setsockopt() SO_REUSEADDR
  int rv;

  struct addrinfo hints, *ai, *p;

  // get us a socket and bind it
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
    log_msg("getaddrinfo %s", gai_strerror(rv));
    goto err;
  }

  for(p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) { 
      continue;
    }

    // lose the pesky "address already in use" error message
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener);
      continue;
    }

    break;
  }

  // if we got here, it means we didn't get bound
  if (p == NULL) {
    log_msg("failed to bind server - alles belegt???");
    goto err;
  }

  // listen
  if (listen(listener, MAX_CONNECTION) == -1) {
    log_perror("listen");
    close(listener);
    goto err;
  }

  freeaddrinfo(ai); // all done with this
    
  return listener;
  
err:
  freeaddrinfo(ai); // all done with this
  return -1;
}



void net_timer() {
  log_msg("timer hat zugeschlagen!!!");
}

int net_pipe(int fd) {
  char buffer[128];
  ssize_t len;
  len = read(fd, buffer, sizeof(buffer));
  if (len == -1) { log_perror("read"); exit(EXIT_FAILURE); }
  
  if (len == 0) {
    log_msg("keine verbindung zum vater - kann socket read hat laenge 0");
    return -1;
  }

  log_msg("neue sachen zum verteilen (%.*s)...", len, buffer);

  fprintf(stderr, "pipe talk: notify all clients\n");

  return 0;
}

void net_client_connect(int fd, fd_set* master, int* fdmax) {

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
}

void net_client_talk(int fd, fd_set* master) {

  char buffer[512];
  // ssize_t len = read(fd, buffer, sizeof(buffer));
  ssize_t len = recv(fd, buffer, sizeof(buffer), 0);

  if (len == 0) {
    // client mag nicht mehr mit uns reden
    log_msg("<%d> client hang up", fd);
    close(fd);
    FD_CLR(fd, master);
    return;
  }

  log_msg("<%d> client sagt >%.*s<", fd, len, buffer);
}



void net_client_loop(int pipe_fd, int net_fd) {
  fd_set rfds, rfds_tmp;
  struct timeval tv;
  int ret;
 
  FD_ZERO(&rfds);
  FD_SET(pipe_fd, &rfds);
  FD_SET(net_fd, &rfds);
  
  int max_fd = MAX(pipe_fd, net_fd);

  for(;;) {

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    rfds_tmp = rfds;
    ret = select(max_fd + 1, &rfds_tmp, NULL, NULL, &tv);

    fprintf(stderr, "-------------------------\n");

    if (ret == -1) { log_perror("select"); exit(EXIT_FAILURE); }

    if (ret == 0) {
      net_timer();
    } else {
      for (int fd = 0; fd <= max_fd; ++fd) {
        if (FD_ISSET(fd, &rfds_tmp)) {
          if (fd == pipe_fd) {
            ret = net_pipe(fd);
            fprintf(stderr, "pipe gelesen: %d\n", ret);
            if (ret == -1) { return; } // return on closed pipe
          } else if (fd == net_fd) {
            // client connect
            net_client_connect(net_fd, &rfds, &max_fd);
            fprintf(stderr, "net_client_connect ferig... max_fd = %d\n", max_fd);
          } else {
            // client sagt irgendwas...
            net_client_talk(fd, &rfds);
            fprintf(stderr, "net_client_talk fertig...\n");
          }
          break;
        } // FD_SET
      } // for
    } // else
  } // while
}


void net_init() {
  if (pipe(net.pipefd) == -1) { log_perror("pipe"); exit(EXIT_FAILURE); }

  net.ppid = getpid();

  net.current_map = 0;
  int fd = shm_open("/wpn_map0", O_CREAT | O_RDWR, 0700);
  if (fd == -1) { log_perror("shm_open"); exit(EXIT_FAILURE); }

  net.map_fd[net.current_map] = fd;

  if ((net.cpid = fork()) == -1) { log_perror("fork"); exit(EXIT_FAILURE); }

  if (net.cpid == 0) {
    // wir sind der client

    // brauchen wir nicht mehr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
//    close(STDERR_FILENO);
    close(net.pipefd[1]);

    int listener = net_bind(PORT);
    if (listener == -1) { log_msg("kann auf port "PORT" nicht binden... exit"); exit(EXIT_FAILURE); }

    net_client_loop(net.pipefd[0], listener);

    close(net.pipefd[0]);
    _exit(EXIT_SUCCESS);
  }

  close(net.pipefd[0]);
}


#if NET_HAVE_MAIN
int main() {

  log_open("test_net_main.log");
  log_msg("---------------- new start");

  net_init();

  for (int loop = 0; loop < 10; ++loop) {

    for (int i = 0; i < 5; ++i) {
      map_printf("hello, map %d\n", loop);
    }

    map_flush();

    sleep(2);

  }

  sleep(100);

}
#endif









