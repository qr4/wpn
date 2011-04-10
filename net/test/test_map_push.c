//
// das hier hat twist verbrochen :-)
//
// twist@nerd2nerd.org
//
//

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <arpa/inet.h>



#define HOST "localhost"
#define PORT "8080"
#define MAXBUF (1024*16)

#define NO_OF_PARALLEL_FETCHES 8

static void handler(int);
static void download(int, unsigned int, size_t);

int main() {
  struct fetch_param {
    unsigned int wait;
    size_t fetch_size;
    int pid;
  };
  
  struct fetch_param foo[NO_OF_PARALLEL_FETCHES];

  for (int i = 0; i < NO_OF_PARALLEL_FETCHES; ++i) {
    foo[i].wait = 0;
    foo[i].fetch_size = 1024;
  }

  foo[0].wait = 1000 * 50;      foo[0].fetch_size = 1024;
  foo[1].wait = 1000 * 100;     foo[1].fetch_size = 512;
  foo[2].wait = 1000 * 200;     foo[2].fetch_size = 512;
  foo[3].wait = 1000 * 250;     foo[3].fetch_size = 512;
  foo[4].wait = 1000 * 500;     foo[4].fetch_size = 1024;
  foo[5].wait = 1000 * 1000;    foo[5].fetch_size = 512;

  signal(SIGTERM, handler);
  signal(SIGINT, handler);
  signal(SIGHUP, handler);

  for (int i = 0; i < NO_OF_PARALLEL_FETCHES; ++i) {
    if ((foo[i].pid = fork()) == 0) { 
      download(i, foo[i].wait, foo[i].fetch_size);
      foo[i].pid = 0;
      exit(0);
    }
  }

  { // auf alle kinder warten
    int status;
    while(wait(&status) > 0) {}
  }

  printf("\nexit\n\n");
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


static void download(int id, unsigned int usec, size_t count) {
  printf("start id %d as %c, wait %uus, blocksize %lu\n", id, 'a' + id, usec, count);
  fflush(stdout);

  
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  
  bzero(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
    printf("getaddrinfo: %s\n", gai_strerror(rv));
    freeaddrinfo(servinfo); // all done with this structure
    exit(EXIT_FAILURE);
  }

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("connect_to_server client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("connect_to_server client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    printf("connect_to_server client: failed to connect\n");
    freeaddrinfo(servinfo); // all done with this structure
    exit(EXIT_FAILURE);
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));

  freeaddrinfo(servinfo); // all done with this structure
    

  sleep(1); // auf die anderen forks warten

  while(1) {
    char buffer[MAXBUF];
    // irgendwas empfangen
    switch (recv(sockfd, buffer, count, 0)) {
    case -1:
      perror("recv");
      exit(EXIT_FAILURE);
    case 0:
      printf("socket closed\n");
      exit(EXIT_SUCCESS);
    }

    if (usec) {
      // wraten... weil wir ja nicht so schnell sind
      usleep(usec);
    }

    printf("%c", 'a' + id);
    fflush(stdout);
  }

  close(sockfd);

}

static void handler(int signum)
{
  exit(0);
}


