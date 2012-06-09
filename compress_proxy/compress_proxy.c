#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/queue.h>
#include <signal.h>
#include <error.h>
#include <string.h>
#include <getopt.h>

#include <ev.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
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
	size_t capacity;
};

#define REF(x)   do { if ((x) != NULL) { (((x)->refcount)++); } } while (0)
#define UNREF(x) do { if ((x) != NULL) {if (--((x)->refcount) == 0) { free((x)->data); free((x)); }}} while (0)

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
struct client_data_list clients_compressed          = TAILQ_HEAD_INITIALIZER(clients_compressed); // list containing all active clients
struct client_data_list clients_compressed_waiting  = TAILQ_HEAD_INITIALIZER(clients_compressed_waiting);
struct client_data_list clients_raw          = TAILQ_HEAD_INITIALIZER(clients_raw); // list containing all active clients
struct client_data_list clients_raw_waiting  = TAILQ_HEAD_INITIALIZER(clients_raw_waiting);

/*
 * structure containing all necessary information.
 * also part of the clients_compressed or clients_compressed_waiting lists.
 */
struct client_data {
	ev_io *w;                                    // associated watcher
	size_t queued;                               // queue size to detect slow connections
	STAILQ_HEAD(com_data_queue, com_data) queue; // output queue
	struct client_data_list *list;               // pointer to the head of the list currently holding the client
	TAILQ_ENTRY(client_data) list_ctl;           // the "next" pointer for the list (see sys/queue.h)
};


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
	if (buf == NULL || buf->n == 0) return;
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

size_t next_power_of_2(size_t x) {
	x--;
	for (size_t i = 1; i < sizeof(size_t) * 8; i <<= 1) {
		x |= x >> i;
	}
	return x + 1;
}

/*
 * return a new buffer initialized with the data in initial_content.
 * the data will be copied, so the returned buffer can be used without
 * restrictions.
 * Note: the returned buffer will have a reference count of 1, so the calling 
 * funtions will own the buffer already.
 */
static buffer *buffer_new(size_t s, const char *initial_content) {
	buffer *buf = calloc(1, sizeof(buffer));
	if (s != 0 && initial_content != NULL) {
		buf->capacity = next_power_of_2(s);
		buf->data = malloc(sizeof(char) * buf->capacity);
		memcpy(buf->data, initial_content, s);
		buf->n = s;
	}
	REF(buf);
	return buf;
}

static buffer *buffer_append(buffer *buf, size_t s, const char *new_content) {
	if (buf == NULL) {
		buf = buffer_new(s, new_content);
	} else {
		size_t new_size = buf->n + s;
		if (new_size > buf->capacity) {
			buf->capacity = next_power_of_2(new_size);
			buf->data = realloc(buf->data, buf->capacity * sizeof(char));
		}
		memcpy(&buf->data[buf->n], new_content, s);
		buf->n = new_size;
	}

	return buf;
}

#define COMPRESSION_LEVEL 1U
#define COMPRESSION_EXTREME 0U
//#define COMPRESSION_EXTREME LZMA_PRESET_EXTREME
#define COMPRESSION_PRESET ((uint32_t) (COMPRESSION_LEVEL | COMPRESSION_EXTREME))
#define DECOMPRESSION_FLAGS ((uint32_t) (LZMA_CONCATENATED))

typedef enum {
	XZ_COMPRESS,
	XZ_FLUSH,
	XZ_FINISH,
} xz_action;


static buffer *xz_run(lzma_stream *stream, lzma_action action) {
	buffer *ret = NULL;
	lzma_ret ret_xz;
	size_t out_len = BUFFSIZE;
	uint8_t out_buf[out_len];

	do {
		stream->next_out = out_buf;
		stream->avail_out = out_len;

		ret_xz = lzma_code(stream, action);
		if ((ret_xz != LZMA_OK) && (ret_xz != LZMA_STREAM_END) && (ret_xz != LZMA_BUF_ERROR)) {
			fprintf(stderr, "%s failed, code %d\n", __func__, ret_xz);
		}
		ret = buffer_append(ret, out_len - stream->avail_out, (char *) out_buf);
	} while (stream->avail_out == 0);

	if (action == LZMA_FINISH) {
		lzma_end(stream);
	}

	return ret;
}

/*
 * easy access to compression with xz.
 * currently only one stream is possible, as the state is maintained
 * throuth static variables.
 */
static buffer *compress_xz(buffer *input, xz_action action) {
	static lzma_stream stream = LZMA_STREAM_INIT;
	static bool need_init = true;
	lzma_ret ret_xz;
	lzma_action xz_action = LZMA_RUN;

	if (need_init) {
		ret_xz = lzma_easy_encoder(&stream, COMPRESSION_PRESET, LZMA_CHECK_CRC64);
		fprintf(stderr, "XZ: new stream\n");
		if (ret_xz != LZMA_OK) {
			fprintf(stderr, "%s: init failed\n", __func__);
		} else {
			need_init = false;
		}
	}

	switch (action) {
		case XZ_COMPRESS :
			xz_action = LZMA_RUN;
			break;
		case XZ_FLUSH :
			xz_action = LZMA_SYNC_FLUSH;
			break;
		case XZ_FINISH :
			xz_action = LZMA_FINISH;
			fprintf(stderr, "XZ: finish stream\n");
			need_init = true;
			break;
		default :
			fprintf(stderr, "%s: wrong action", __func__);
			break;
	}

	if (input == NULL || input->n == 0) {
		stream.next_in = NULL;
		stream.avail_in = 0;
	} else {
		stream.next_in = (const uint8_t *) input->data;
		stream.avail_in = input->n;
	}

	return xz_run(&stream, xz_action);
}


static buffer *decompress_xz(buffer *input, xz_action action) {
	static lzma_stream stream = LZMA_STREAM_INIT;
	static bool need_init = true;
	lzma_ret ret_xz;
	const size_t mem_limit = UINT64_MAX;

	if (need_init) {
		need_init = false;
		ret_xz = lzma_stream_decoder(&stream, mem_limit, DECOMPRESSION_FLAGS);
		if (ret_xz != LZMA_OK) {
			fprintf(stderr, "lzme_stream_decoder error: %d\n", (int) ret_xz);
			need_init = true;
			return buffer_new(0, NULL);
		}
	}

	if (action == XZ_FINISH) {
		need_init = true;
	}

	if (input == NULL || input->n == 0) {
		stream.next_in = NULL;
		stream.avail_in = 0;
	} else {
		stream.next_in = (const uint8_t *) input->data;
		stream.avail_in = input->n;
	}

	return xz_run(&stream, (action == XZ_FINISH) ? LZMA_FINISH : LZMA_RUN);
}

static void compress_and_broadcast(buffer *buf, xz_action action) {
	// only broadcast if there are clients in the queue
	if (TAILQ_EMPTY(&clients_compressed) && action != XZ_FINISH) return;
	REF(buf);
	buffer *out = compress_xz(buf, action);
	broadcast(EV_DEFAULT_ &clients_compressed, out);
	UNREF(out);
	UNREF(buf);
}

static void concat_queues(struct client_data_list *dest, struct client_data_list *src) {
	if (dest == src || TAILQ_EMPTY(src)) return;

	client_data *cd;
	TAILQ_FOREACH(cd, src, list_ctl) {
		cd->list = dest;
	}
	TAILQ_CONCAT(dest, src, list_ctl);
	printf("adding clients\n");
}

/*
 * called when the gameserver sends new data
 *
 * the plan:
 *
 * if clients_compressed_waiting is empty:
 * take the new input and compress with it with xz and broadcast all
 * available encoded data.
 *
 * if clients_compressed_waiting is _not_ empty:
 * behave as clients_compressed_waiting was empty, if there is not a complete world end.
 * otherwise encode only to end of the last complete world, flush the
 * encoder and broadcast to active clients the rest of the encoded stream.
 * then add all clients from clients_compressed_waiting to the active list and begin a
 * new stream, encode what's left after the last complete world and broadcast
 * the new stream to the new active list
 *
 * possible addition:
 * wait a few turns to add new clients, to increase compression performance.
 * you also lost the game.
 */
static const uint8_t XZ_HEADER_MAGIC[6] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };

static void server_read_cb(EV_P_ ev_io *w, int revents) {
	static size_t depth = 0; // used to decide when a world is fully sent
	static bool first_read = true;
	static bool is_compressed_input = false;
	buffer *buf;
	ssize_t r;
	char t_buf[BUFFSIZE];
	xz_action action = XZ_COMPRESS;
	buffer *input = NULL;

	if (first_read) {
		// detect if incoming data is raw or compressed
		first_read = false;

		for (size_t i = 0; i < sizeof(XZ_HEADER_MAGIC); i++) {
			r = read(w->fd, &t_buf[i], 1);
			if (r == 0 || r == -1) { goto error; }
		}

		r = sizeof(XZ_HEADER_MAGIC);

		if (memcmp(t_buf, XZ_HEADER_MAGIC, sizeof(XZ_HEADER_MAGIC)) == 0) {
			is_compressed_input = true;
			printf("Detected xz stream\n");
		} else {
			printf("Detected raw stream\n");
		}
	} else {
		r = read(w->fd, t_buf, BUFFSIZE); // try to fill t_buf
		if (r == 0 || r == -1) { goto error; }
	}

	input = buffer_new(r, t_buf);

	if (is_compressed_input) {
		buffer *t = input;
		input = decompress_xz(t, XZ_COMPRESS);
		UNREF(t);
	}

	// detect where in the stream a world is complete. if there are
	// multiple complete worlds, only mark the last end
	ssize_t latest_world_end = -1;
	for (size_t i = 0; i < input->n; i++) {
		switch (input->data[i]) {
			case '{':
				depth++;
				break;
			case '}':
				depth--;
				if (depth == 0) {
					//printf("End of the world detected!\n");
					latest_world_end = i;
					action = XZ_FLUSH;
				}
				break;
			default:
				if (depth == 0) {
					latest_world_end++;
				}
				break;
		}
	}


	/*
	 * process the compressed queue
	 */
	ssize_t written = 0;
	if (!TAILQ_EMPTY(&clients_compressed_waiting) && latest_world_end != -1) {
		action = XZ_FINISH;

		buf = buffer_new(latest_world_end, input->data);
		compress_and_broadcast(buf, action);
		UNREF(buf);

		concat_queues(&clients_compressed, &clients_compressed_waiting);
		written = latest_world_end;

		action = XZ_COMPRESS; // we flushed already
	}

	if (r > written) {
		buf = buffer_new(input->n - written, &input->data[written]);
		compress_and_broadcast(buf, action);
		UNREF(buf);
	}

	/*
	 * process the raw queue
	 * meh, code duplication, yes, i'll fix, maybe, whatever
	 */
	written = 0;
	if (!TAILQ_EMPTY(&clients_raw_waiting) && latest_world_end != -1) {
		buf = buffer_new(latest_world_end, input->data);
		broadcast(EV_DEFAULT_ &clients_raw, buf);
		UNREF(buf);

		concat_queues(&clients_raw, &clients_raw_waiting);
		written = latest_world_end;
	}

	if (r > written) {
		buf = buffer_new(input->n - written, &input->data[written]);
		broadcast(EV_DEFAULT_ &clients_raw, buf);
		UNREF(buf);
	}

	UNREF(input);

	return;

error:
	fprintf(stderr, "Error in %s\n", __func__);
	fprintf(stderr, "Connection closed\n");
	ev_break(EV_DEFAULT_ EVBREAK_ALL);
	depth = 0;
	first_read = true;
}

typedef struct {
	struct sockaddr_storage sin;
	socklen_t slen;
	int fd;
	struct client_data_list *list;
} bind_info;

/*
 * called when a new client connects to the proxy.
 * initializes all necessary data for the client (watcher, output queue, etc..)
 * and adds it to clients_compressed_waiting.
 */
static void accept_cb(EV_P_ ev_io *w, int revents) {
	client_data *cd;
	int fd;
	bind_info *binfo = w->data;

	printf("incoming connection\n");

	if ((fd = accept(binfo->fd, (struct sockaddr *) &binfo->sin, &binfo->slen)) == -1) perror("accept");
	set_nonblocking(fd);

	cd = malloc(sizeof(client_data));
	cd->w = malloc(sizeof(ev_io));
	cd->list = binfo->list;
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
static bind_info *do_bind(const char *hostname, const char *port) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	bind_info *ret = NULL;
	int rv;
	int t = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	fprintf(stderr, "bind on %s:%s\n", hostname, port);
	rv = getaddrinfo(hostname, port, &hints, &servinfo);

	if (rv != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(rv));
		return NULL;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { goto error; }
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int))) { goto error; }
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) != 0) { goto error; }
		if (listen(sockfd, 128) != 0) { goto error; }

		ret = calloc(1, sizeof(bind_info));
		ret->slen = p->ai_addrlen;
		memcpy(&ret->sin, p->ai_addr, ret->slen);
		ret->fd = sockfd;
		ret->list = NULL;

		break;
error:
		close(sockfd);
		perror("connect_to_server client: setsockopt");
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL) {
		free(ret);
		printf("connect_to_server client: failed to bind in %s:%s\n", hostname, port);
		return NULL;
	}

	return ret;
}

/*
 * to be able to shutdown the proxy, wait until stdin is closed. if so, close
 * all connections and stop the event_loop
 */
static void stdin_cb(EV_P_ ev_io *w, int revents) {
	const size_t buffsize = 1024;
	char stdin_buffer[buffsize];

	ssize_t r = read(w->fd, stdin_buffer, buffsize);
	if (r == 0) {
		ev_break(EV_DEFAULT_ EVBREAK_ALL);
	}
}

/*
 * from viewer, some network magic, i didn't write it, i didn't make efforts
 * to understand it.
 */
static int connect2server(const char* server, const char* port) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	fprintf(stderr, "connect to server %s:%s\n", server, port);
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


/*
 * optionparsing
 */
typedef struct {
	char *hostname;
	char *port;
} hostinfo;

static struct {
	hostinfo *input;
	hostinfo *raw_out;
	hostinfo *xz_out;
	bool listen_mode;
	bool enable_raw;
	bool enable_xz;
} all_options;

static void free_hostinfo(hostinfo *hi) {
	if (hi == NULL) return;
	free(hi->hostname);
	free(hi->port);
	free(hi);
}

static hostinfo *parse_hostinfo(const char* hostname) {
	hostinfo *ret = calloc(1, sizeof(all_options));
	ret->hostname = strdup(hostname);
	char *port = rindex(ret->hostname, ':');
	if (port != NULL) {
		ret->port = port;
		ret->port[0] = '\0';
		ret->port = strdup(++ret->port);
	}

	if ( ret->port == NULL || atoi(ret->port) == 0) {
		fprintf(stderr, "Invalid Host: %s\n", hostname);
		free_hostinfo(ret);
		ret = NULL;
		exit(1);
	}

	return ret;
}

static void print_hostinfo(const hostinfo *hi, const char *type) {
	if (hi == NULL) return;
	printf("%s host:  %s:%s\n", type, hi->hostname, hi->port);
}

static void cleanup_options() {
	free_hostinfo(all_options.input);
	free_hostinfo(all_options.raw_out);
	free_hostinfo(all_options.xz_out);
}

void print_usage(char *argv[]) {
	fprintf(stderr, "Compression Proxy\n\n");
	fprintf(stderr, "Usage: %s -h|--host host:port [-r|--raw host:port] [-x|--xz host:port]\n\n", argv[0]);
	fprintf(stderr, "-h,--host     Connect to host:port for input\n");
	fprintf(stderr, "-r,--raw      Open socket on host:port for raw multicast (Optional)\n");
	fprintf(stderr, "-x,--xz       Open socket on host:port for compressed xz multicast (Optional)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "You must specify a reachable input host and at least one output (-r or -x)\n");
	fprintf(stderr, "\n");

}

void parse_config(int argc, char *argv[]) {
    const struct option options[] = {
		{"listen" , required_argument , NULL , 'l'},
		{"host"   , required_argument , NULL , 'h'},
		{"raw"    , required_argument , NULL , 'r'},
		{"xz"     , required_argument , NULL , 'x'},
		{0        , 0                 , 0    , 0}
    };

	if (argc < 3) {
		print_usage();
		return;
	}

	cleanup_options();
	atexit(cleanup_options);

    int c;

    while ((c = getopt_long(argc, argv, "l:h:r:x:", options, NULL)) != -1) {
		//printf("Got %c with \"%s\"\n", c, optarg);
        switch (c) {
            case 'l' : // Change to listen mode
				if (optarg) {
					free_hostinfo(all_options.input);
					all_options.input = parse_hostinfo(optarg);
				}
				all_options.listen_mode = true;
                break;
            case 'h' : // Set remote host
				if (optarg) {
					free_hostinfo(all_options.input);
					all_options.input = parse_hostinfo(optarg);
				}
				all_options.listen_mode = false;
                break;
			case 'r' : // Enable raw output and set address:port to bind to
				if (optarg) {
					free_hostinfo(all_options.raw_out);
					all_options.raw_out = parse_hostinfo(optarg);
				}
				all_options.enable_raw = true;
                break;
            case 'x' :
				if (optarg) {
					free_hostinfo(all_options.xz_out);
					all_options.xz_out = parse_hostinfo(optarg);
				}
				all_options.enable_xz = true;
				break;
            case ':' :
                break;
            default :
                // Unknown option or missing param
                break;
        }
    }
	print_hostinfo(all_options.input, "input");
	print_hostinfo(all_options.raw_out, "raw_out");
	print_hostinfo(all_options.xz_out, "xz_out");

	printf("listen_mode: %s\n", all_options.listen_mode ? "true" : "false");
	printf("enable_raw:  %s\n", all_options.enable_raw ? "true" : "false");
	printf("enable_xz:   %s\n", all_options.enable_xz ? "true" : "false");

	for (; optind < argc; optind++) {
		printf("Unused paramenter %s\n", argv[optind]);
	}

	if (all_options.input == NULL) {
		fprintf(stderr, "No active input!\n");
	}

	if ((all_options.raw_out == NULL && all_options.xz_out == NULL)
			|| (!all_options.enable_raw && !all_options.enable_xz)) {
		fprintf(stderr, "No active output!\n");
		exit(1);
	}
}

void cleanup_xz() {
	buffer *buf;
	buf = compress_xz(NULL, XZ_FINISH);
	UNREF(buf);
	buf = decompress_xz(NULL, XZ_FINISH);
	UNREF(buf);

}

int main (int argc, char *argv[]) {
	parse_config(argc, argv);

	int server_fd;
	bind_info *binfo_xz = NULL;
	bind_info *binfo_raw = NULL;

	server_fd = connect2server(all_options.input->hostname, all_options.input->port);
	if (server_fd == -1) {
		perror("connect");
		goto connection_cleanup;
	}

	if (all_options.enable_raw) {
		binfo_raw = do_bind(all_options.raw_out->hostname, all_options.raw_out->port);
		if (binfo_raw == NULL) { goto connection_cleanup; };
		binfo_raw->list = &clients_raw_waiting;
	}

	if (all_options.enable_xz) {
		binfo_xz = do_bind(all_options.xz_out->hostname, all_options.xz_out->port);
		if (binfo_xz == NULL) { goto connection_cleanup; };
		binfo_xz->list = &clients_compressed_waiting;
	}


	atexit(cleanup_xz);

	signal(SIGPIPE, sigpipe_ignore); // ignore SIGPIPE

	// setup all initial watchers
	ev_io server_watcher;
	ev_io_init(&server_watcher, server_read_cb, server_fd, EV_READ);
	ev_io_start(EV_DEFAULT_ &server_watcher);

	ev_io stdin_watcher;
	ev_io_init(&stdin_watcher, stdin_cb, fileno(stdin), EV_READ);
	ev_io_start(EV_DEFAULT_ &stdin_watcher);

	ev_io raw_watcher;
	raw_watcher.data = binfo_raw;
	ev_io_init(&raw_watcher, accept_cb, binfo_raw->fd, EV_READ);
	ev_io_start(EV_DEFAULT_ &raw_watcher);

	ev_io xz_watcher;
	xz_watcher.data = binfo_xz;
	ev_io_init(&xz_watcher, accept_cb, binfo_xz->fd, EV_READ);
	ev_io_start(EV_DEFAULT_ &xz_watcher);

	// run the event loop
	ev_run (EV_DEFAULT_ 0);

	// close all connections and flush the encoder
	{
		client_data    *cd, *cd_tmp;
		TAILQ_FOREACH_SAFE(cd, &clients_compressed, list_ctl, cd_tmp) {
			close_connection(EV_DEFAULT_ cd->w);
		}
		TAILQ_FOREACH_SAFE(cd, &clients_compressed_waiting, list_ctl, cd_tmp) {
			close_connection(EV_DEFAULT_ cd->w);
		}
		TAILQ_FOREACH_SAFE(cd, &clients_raw, list_ctl, cd_tmp) {
			close_connection(EV_DEFAULT_ cd->w);
		}
		TAILQ_FOREACH_SAFE(cd, &clients_raw_waiting, list_ctl, cd_tmp) {
			close_connection(EV_DEFAULT_ cd->w);
		}
	}

	// close down everything
	ev_io_stop(EV_DEFAULT_ &server_watcher);
	ev_io_stop(EV_DEFAULT_ &stdin_watcher);
	ev_io_stop(EV_DEFAULT_ &raw_watcher);
	ev_io_stop(EV_DEFAULT_ &xz_watcher);
	ev_loop_destroy(EV_DEFAULT);

connection_cleanup:
	if (binfo_xz != NULL) { close(binfo_xz->fd); }
	free(binfo_xz);

	if (binfo_raw != NULL) { close(binfo_raw->fd); }
	free(binfo_raw);

	if (server_fd >= 0) { close(server_fd); }

	// cya
	return 0;
}
