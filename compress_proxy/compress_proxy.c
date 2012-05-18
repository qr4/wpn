#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/queue.h>
#include <signal.h>
#include <error.h>
#include <string.h>

#include <ev.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFFSIZE 4096
#define MAX_BUF 1024*1024

void sigpipe_ignore(int n) {}

typedef struct buffer buffer;
typedef struct client_data client_data;
typedef struct client_data_list client_data_list;
typedef struct com_data com_data;
typedef struct com_data_queue com_data_queue;

struct buffer {
	char *data;
	size_t n;
	size_t refcount;
};

#define REF(x)   (((x)->refcount)++)
#define UNREF(x) do { if (--((x)->refcount) == 0) { free((x)->data); free((x)); }} while (0)

struct com_data {
	buffer *buffer;
	size_t start;
	STAILQ_ENTRY(com_data) list_ctl;
};

TAILQ_HEAD(client_data_list, client_data);
struct client_data_list clients_all          = TAILQ_HEAD_INITIALIZER(clients_all);
//struct client_data_list clients_waiting      = TAILQ_HEAD_INITIALIZER(clients_waiting);
//struct client_data_list clients_small_blocks = TAILQ_HEAD_INITIALIZER(clients_small_blocks);
//struct client_data_list clients_large_blocks = TAILQ_HEAD_INITIALIZER(clients_large_blocks);

struct client_data {
	ev_io *w;
	size_t queued;
	STAILQ_HEAD(com_data_queue, com_data) queue;
	struct client_data_list *list;
	TAILQ_ENTRY(client_data) list_ctl;
};

static struct sockaddr_in sin;
static socklen_t slen;

int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFD);
	if (flags == -1) goto error;
	if (fcntl(fd, F_SETFD, flags | O_NONBLOCK) == -1) goto error;

	return 0;

error:
	perror("set_nonblocking");
	return -1;
}

static void close_connection(EV_P_ ev_io *w) {
	client_data *cd = (client_data *) w->data;
	com_data_queue *queue = &cd->queue;

	fprintf(stderr, "closing connection\n");
	ev_io_stop(EV_DEFAULT_ w);
	close(w->fd);

	while (!STAILQ_EMPTY(queue)) {
		com_data *t = STAILQ_FIRST(queue);
		STAILQ_REMOVE_HEAD(queue, list_ctl);
		UNREF(t->buffer);
		free(t);
	}

	TAILQ_REMOVE(cd->list, cd, list_ctl);
	free(w);
	free(cd);
}

static void client_write_cb(EV_P_ ev_io *w, int revents) {
	ssize_t written;
	client_data *cd = (client_data *) w->data;
	com_data_queue *queue = &cd->queue;
	com_data *c_data = STAILQ_FIRST(queue);
	buffer *buf = c_data->buffer;

	written = write(w->fd, &buf->data[c_data->start], buf->n - c_data->start);
	if (written == -1) {
		close_connection(EV_DEFAULT_ w);
		return;
	}

	c_data->start += written;
	cd->queued -= written;

	if (buf->n == c_data->start) {
		STAILQ_REMOVE_HEAD(queue, list_ctl);
		UNREF(buf);
		free(c_data);

		if (STAILQ_EMPTY(queue)) {
			ev_io_stop(EV_DEFAULT_ w);
		}
	}

	return;
}

static void broadcast(EV_P_ client_data_list *clients, buffer *buf) {
	client_data    *cd, *cd_tmp;
	com_data       *c_data;
	TAILQ_FOREACH_SAFE(cd, clients, list_ctl, cd_tmp) {
		if (cd->queued + buf->n > MAX_BUF) {
			printf("buffer threshhold\n");
			close_connection(EV_DEFAULT_ cd->w);
			continue;
		}

		c_data = malloc(sizeof(com_data));
		c_data->start = 0;
		c_data->buffer = buf;
		REF(buf);

		if (STAILQ_EMPTY(&cd->queue)) {
			ev_io_stop(EV_DEFAULT_ cd->w);
			ev_io_set(cd->w, cd->w->fd, EV_READ | EV_WRITE);
			ev_io_start(EV_DEFAULT_ cd->w);
		}

		STAILQ_INSERT_TAIL(&cd->queue, c_data, list_ctl);
		cd->queued += buf->n;
	}
}

static void server_read_cb(EV_P_ ev_io *w, int revents) {
	static size_t depth = 0;
	buffer *buf;
	ssize_t r;
	char t_buf[BUFFSIZE];

	r = read(w->fd, t_buf, BUFFSIZE);
	if (r == 0 || r == -1) {
		// TODO
		depth = 0;
		return;
	}

	size_t latest_world_end = 0;
	for (size_t i = 0; i < r; i++) {
		switch (t_buf[i]) {
			case '{':
				depth++;
				break;
			case '}':
				depth--;
				if (depth == 0) {
					printf("End of the world detected!\n");
					latest_world_end = i;
				}
				break;
			default:
				if (depth == 0) {
					latest_world_end++;
				}
				break;
		}
	}


	buf = calloc(1, sizeof(buffer));
	buf->data = malloc(sizeof(char) * r);
	memcpy(buf->data, t_buf, r);
	buf->n = r;
	REF(buf);

	broadcast(EV_DEFAULT_ &clients_all, buf);
	UNREF(buf);

	return;
}

static void accept_cb(EV_P_ ev_io *w, int revents) {
	client_data *c_data;
	int fd;

	printf("incoming connection\n");

	if ((fd = accept(w->fd, (struct sockaddr *) &sin, &slen)) == -1) perror("accept");
	set_nonblocking(fd);

	c_data = malloc(sizeof(client_data));
	c_data->w = malloc(sizeof(ev_io));
	c_data->list = &clients_all;
	c_data->queued = 0;
	ev_io_init(c_data->w, client_write_cb, fd, EV_WRITE);
	STAILQ_INIT(&(c_data->queue));
	TAILQ_INSERT_TAIL(c_data->list, c_data, list_ctl);

	c_data->w->data = c_data;
}

static int do_bind() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int t = 1;

	if (sockfd < 0) perror("Error opening socket.");
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int))) perror ("setsockopt");

	slen = sizeof(sin);
	memset(&sin, 0, slen);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0);
	sin.sin_port = htons(8081);

    if(bind(sockfd, (struct sockaddr *) &sin, slen) != 0) perror("bind");
	if(listen(sockfd, 128) != 0) perror("listen");

	return sockfd;
}

static void stdin_cb(EV_P_ ev_io *w, int revents) {
	const size_t buffsize = 1024;
	char buffer[buffsize];

	ssize_t r = read(w->fd, buffer, buffsize);
	if (r == 0) {
		client_data    *cd, *cd_tmp;
		TAILQ_FOREACH_SAFE(cd, &clients_all, list_ctl, cd_tmp) {
			close_connection(EV_DEFAULT_ cd->w);
		}
		ev_break(EV_DEFAULT_ EVBREAK_ALL);
	}
}

// from viewer
static int connect2server(const char* server, const char* port) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	printf("connect to server %s:%s\n", server, port);
	rv = getaddrinfo(server, port, &hints, &servinfo);

	if (rv != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
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

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL) {
		printf("connect_to_server client: failed to connect\n");
		return -1;
	}

	return sockfd;
}

int main (void) {
	int sockfd = do_bind();
	int server_fd = connect2server("localhost", "8080");

	if (server_fd == -1) {
		perror("connect");
		exit(1);
	}

	signal(SIGPIPE, sigpipe_ignore);

	ev_io server_watcher;
	ev_io_init(&server_watcher, server_read_cb, server_fd, EV_READ);
	ev_io_start(EV_DEFAULT_ &server_watcher);

	ev_io stdin_watcher;
	ev_io_init(&stdin_watcher, stdin_cb, fileno(stdin), EV_READ);
	ev_io_start(EV_DEFAULT_ &stdin_watcher);

	ev_io bind_watcher;
	ev_io_init(&bind_watcher, accept_cb, sockfd, EV_READ);
	ev_io_start(EV_DEFAULT_ &bind_watcher);

	ev_run (EV_DEFAULT_ 0);

	ev_io_stop(EV_DEFAULT_ &server_watcher);
	ev_io_stop(EV_DEFAULT_ &stdin_watcher);
	ev_io_stop(EV_DEFAULT_ &bind_watcher);
	ev_loop_destroy(EV_DEFAULT);

	close(server_fd);
	close(sockfd);

	return 0;
}
