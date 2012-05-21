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
#include <lzma.h>

#define BUFFSIZE 4096
#define MAX_BUF 1024*1024

void sigpipe_ignore(int n) {}

typedef struct buffer buffer;
typedef struct client_data client_data;
typedef struct client_data_list client_data_list;
typedef struct com_data com_data;
typedef struct com_data_queue com_data_queue;

/*
 * output buffer that is shared using com_data
 * use REF() when sharing a buffer or UNREF() when the buffer is not used by one
 * owner anymore.
 */
struct buffer {
	char *data;
	size_t n;        // size of the buffer
	size_t refcount; // do not modify manually, use REF / UNREF macros
};

#define REF(x)   (((x)->refcount)++)
#define UNREF(x) do { if (--((x)->refcount) == 0) { free((x)->data); free((x)); }} while (0)

/*
 * a single entry for an output queue of one client.
 * keeps track of the bytes already sent to the client.
 */
struct com_data {
	buffer *buffer; // the shared buffer
	size_t start;   // position from where to start next
	STAILQ_ENTRY(com_data) list_ctl;
};

TAILQ_HEAD(client_data_list, client_data); // type definition
struct client_data_list clients_all          = TAILQ_HEAD_INITIALIZER(clients_all); // list containing all active clients
//struct client_data_list clients_waiting      = TAILQ_HEAD_INITIALIZER(clients_waiting);
//struct client_data_list clients_small_blocks = TAILQ_HEAD_INITIALIZER(clients_small_blocks);
//struct client_data_list clients_large_blocks = TAILQ_HEAD_INITIALIZER(clients_large_blocks);

/*
 * structure containing all necessary information.
 * also part of the clients_all or clients_waiting lists.
 */
struct client_data {
	ev_io *w;                                    // associated watcher
	size_t queued;                               // queue size to detect slow connections
	STAILQ_HEAD(com_data_queue, com_data) queue; // output queue
	struct client_data_list *list;               // pointer to the head of the list currently holding the client
	TAILQ_ENTRY(client_data) list_ctl;           // the "next" pointer for the list (see sys/queue.h)
};

static struct sockaddr_in sin;
static socklen_t slen;

/*
 * helper function
 */
int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFD);
	if (flags == -1) goto error;
	if (fcntl(fd, F_SETFD, flags | O_NONBLOCK) == -1) goto error;

	return 0;

error:
	perror("set_nonblocking");
	return -1;
}

/*
 * close a connection and remove all associated data to it.
 */
static void close_connection(EV_P_ ev_io *w) {
	client_data *cd = (client_data *) w->data;
	com_data_queue *queue = &cd->queue;

	fprintf(stderr, "closing connection\n"); // debug

	ev_io_stop(EV_DEFAULT_ w); // remove from event loop
	close(w->fd);              // close connection

	// free the output queue
	while (!STAILQ_EMPTY(queue)) {
		com_data *t = STAILQ_FIRST(queue);
		STAILQ_REMOVE_HEAD(queue, list_ctl);
		UNREF(t->buffer);
		free(t);
	}

	TAILQ_REMOVE(cd->list, cd, list_ctl); // remove client from its broadcast list
	free(w);
	free(cd);
}

/*
 * called when there is data in the output buffer and
 * the socket is writeable or if the socket is marked readable
 * will attempt to send from one buffer as much as possible, but will not try to 
 * send the complete output queue to avoid starvation of other clients
 */
static void client_cb(EV_P_ ev_io *w, int revents) {
	/*
	 * if there's something to read, just read it and close connection
	 * on error. This should be the case, when the clients closes
	 * the connection.
	 *
	 * TODO:
	 * better spam detection
	 */
	if (revents & EV_READ) {
		char t_buf[BUFFSIZE];
		ssize_t r = read(w->fd, t_buf, BUFFSIZE);
		printf("read something (%ld)\n", r);
		if (r == -1 || r == 0 || r == BUFFSIZE) {
			close_connection(EV_DEFAULT_ w);
			return;
		}
	}

	if (revents & EV_WRITE) {
		ssize_t written;
		client_data *cd = (client_data *) w->data;
		com_data_queue *queue = &cd->queue;
		com_data *c_data = STAILQ_FIRST(queue);
		buffer *buf = c_data->buffer;

		// try to write the first buffer in the queue
		written = write(w->fd, &buf->data[c_data->start], buf->n - c_data->start);
		if (written == -1) {
			close_connection(EV_DEFAULT_ w);
			return;
		}

		// keep track of how much was written
		c_data->start += written;
		cd->queued -= written;


		// buffer fully sent?
		if (buf->n == c_data->start) {
			// then remove it
			STAILQ_REMOVE_HEAD(queue, list_ctl);
			UNREF(buf);
			free(c_data);

			// output queue is empty?
			if (STAILQ_EMPTY(queue)) {
				// stop the watcher and watch reads only, as there is nothing to send
				ev_io_stop(EV_DEFAULT_ w);
				ev_io_set(cd->w, cd->w->fd, EV_READ);
				ev_io_start(EV_DEFAULT_ w);
			}
		}
	}
}

/*
 * enqueue a buffer to all clients in the active list.
 */
static void broadcast(EV_P_ client_data_list *clients, buffer *buf) {
	client_data    *cd, *cd_tmp;
	com_data       *c_data;

	// loop over every client in the active list
	TAILQ_FOREACH_SAFE(cd, clients, list_ctl, cd_tmp) {
		if (cd->queued + buf->n > MAX_BUF) {
			// slow connection detected
			printf("buffer threshhold\n");
			close_connection(EV_DEFAULT_ cd->w);
			continue;
		}

		// new queue element
		c_data = malloc(sizeof(com_data));
		c_data->start = 0;
		c_data->buffer = buf;
		REF(buf);

		if (STAILQ_EMPTY(&cd->queue)) {
			// queue was empty, so the watcher was not started, as there
			// was nothing, that could be written. now start it
			ev_io_stop(EV_DEFAULT_ cd->w);
			// maybe EV_WRITE is enough, or add a read_cb that reads
			// incoming data
			ev_io_set(cd->w, cd->w->fd, EV_READ | EV_WRITE);
			ev_io_start(EV_DEFAULT_ cd->w);
		}

		// now add new element to the queue
		STAILQ_INSERT_TAIL(&cd->queue, c_data, list_ctl);
		// keep track of the enqueued data
		cd->queued += buf->n;
	}
}

/*
 * return a new buffer initialized with the data in initial_content.
 * the data will be copied, so the returned buffer can be used without
 * restrictions.
 * Note: the returned buffer will have a reference count of 1, so the calling 
 * funtions will own the buffer already.
 */
static buffer *new_buffer(size_t s, char *initial_content) {
	buffer *buf = calloc(1, sizeof(buffer));
	if (s != 0 && initial_content != NULL) {
		buf->data = malloc(sizeof(char) * s);
		memcpy(buf->data, initial_content, s);
		buf->n = s;
	}
	REF(buf);
	return buf;
}

/*
 * called when the gameserver sends new data
 */
static void server_read_cb(EV_P_ ev_io *w, int revents) {
	static size_t depth = 0; // used to decide when a world is fully sent
	buffer *buf;
	ssize_t r;
	char t_buf[BUFFSIZE];

	r = read(w->fd, t_buf, BUFFSIZE); // try to fill t_buf
	if (r == 0 || r == -1) {
		// TODO
		depth = 0;
		return;
	}

	// detect where in the stream a world is complete. if there are
	// multiple complete worlds, only mark the last end
	size_t latest_world_end = 0;
	for (size_t i = 0; i < r; i++) {
		switch (t_buf[i]) {
			case '{':
				depth++;
				break;
			case '}':
				depth--;
				if (depth == 0) {
					//printf("End of the world detected!\n");
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
	//printf("(%ld)\n", r);

	/*
	 * the plan:
	 *
	 * if clients_waiting is empty:
	 * take the new input and compress with it with xz and broadcast all
	 * available encoded data.
	 *
	 * if clients_waiting is _not_ empty:
	 * behave as clients_waiting was empty, if there is not a complete world end.
	 * otherwise encode only to end of the last complete world, flush the
	 * encoder and broadcast to active clients the rest of the encoded stream.
	 * then add all clients from clients_waiting to the active list and begin a
	 * new stream, encode what's left after the last complete world and broadcast
	 * the new stream to the new active list
	 *
	 * possible addition:
	 * wait a few turns to add new clients, to increase compression performance.
	 * you also lost the game.
	 */

	/**********
	 * <TODO> *
	 **********/

	buf = new_buffer(latest_world_end, t_buf);
	broadcast(EV_DEFAULT_ &clients_all, buf);
	UNREF(buf);

	if (r <= latest_world_end) return;

	buf = new_buffer(r - latest_world_end, t_buf + latest_world_end);
	broadcast(EV_DEFAULT_ &clients_all, buf);
	UNREF(buf);

	/***********
	 * </TODO> *
	 ***********/
	return;
}

/*
 * called when a new client connects to the proxy.
 * initializes all necessary data for the client (watcher, output queue, etc..)
 * and adds it to clients_waiting.
 */
static void accept_cb(EV_P_ ev_io *w, int revents) {
	client_data *cd;
	int fd;

	printf("incoming connection\n");

	if ((fd = accept(w->fd, (struct sockaddr *) &sin, &slen)) == -1) perror("accept");
	set_nonblocking(fd);

	cd = malloc(sizeof(client_data));
	cd->w = malloc(sizeof(ev_io));
	cd->list = &clients_all; // TODO: change to clients_waiting
	cd->queued = 0;
	ev_io_init(cd->w, client_cb, fd, EV_WRITE);
	STAILQ_INIT(&(cd->queue));
	TAILQ_INSERT_TAIL(cd->list, cd, list_ctl);

	cd->w->data = cd;

	// listen on incoming data
	ev_io_set(cd->w, cd->w->fd, EV_READ);
	ev_io_start(EV_DEFAULT_ w);
}

/*
 * nothing special, return a functioning server socket
 */
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

/*
 * to be able to shutdown the proxy, wait until stdin is closed. if so, close
 * all connections and stop the event_loop
 */
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

/*
 * from viewer, some network magic, it didn't write it, i didn't make effords
 * to understand it.
 */
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
	int sockfd = do_bind(); // open the server socket
	int server_fd = connect2server("localhost", "8080"); // connect to the server

	if (server_fd == -1) {
		perror("connect");
		exit(1);
	}

	signal(SIGPIPE, sigpipe_ignore); // ignore SIGPIPE

	// setup all initial watchers
	ev_io server_watcher;
	ev_io_init(&server_watcher, server_read_cb, server_fd, EV_READ);
	ev_io_start(EV_DEFAULT_ &server_watcher);

	ev_io stdin_watcher;
	ev_io_init(&stdin_watcher, stdin_cb, fileno(stdin), EV_READ);
	ev_io_start(EV_DEFAULT_ &stdin_watcher);

	ev_io bind_watcher;
	ev_io_init(&bind_watcher, accept_cb, sockfd, EV_READ);
	ev_io_start(EV_DEFAULT_ &bind_watcher);

	// run the event loop
	ev_run (EV_DEFAULT_ 0);

	// close down everything
	ev_io_stop(EV_DEFAULT_ &server_watcher);
	ev_io_stop(EV_DEFAULT_ &stdin_watcher);
	ev_io_stop(EV_DEFAULT_ &bind_watcher);
	ev_loop_destroy(EV_DEFAULT);

	close(server_fd);
	close(sockfd);

	// cya
	return 0;
}
