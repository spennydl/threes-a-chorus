#include "server.h"
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief A queued request.
 */
typedef struct _rq
{
    /** The sender's address. */
    struct sockaddr_in remote_addr;
    /** Length of the sender's address. */
    socklen_t addr_len;
    /** Next request in the queue. */
    struct _rq* next;
    /** The request packet. */
    request_data* data;
} _request;

/**
 * @brief Convenience struct holding a request handler and its user data.
 */
typedef struct
{
    /** Request handler function. */
    handler_fn* handler;
    /** Request handler user data. */
    void* usr_data;
} _user_request_handler;

/**
 * @brief Internal server structure.
 */
typedef struct
{
    /** Socket to listen on for requests. */
    int sock_fd;
    /** Current server status. */
    _Atomic int status;
    /** Handle to the request handler thread. */
    pthread_t request_handler;
    /** Handle to the request listener thread. */
    pthread_t listener;
    /** Lock for the request queue. */
    pthread_mutex_t rq_lock;
    /**
     * Condition lock that protects the condition of whether or not there are
     * requests in the queue.
     */
    pthread_cond_t rq_ready;
    /** The request queue. */
    _request* rq;
    /** The request handler. */
    _user_request_handler usr_handler;
} _server;

/**
 * @brief Opens a socket and optionally binds it to a local address.
 *
 * @param s_in The local address.
 * @param s_len The length of the local address.
 * @param bind_local Whether or not to bind to a local address.
 */
static int
_server_open_udp_socket(struct sockaddr* s_in,
                        socklen_t s_len,
                        bool bind_local);
/**
 * @brief Sends a UDP datagram to a remote address.
 *
 * @param server The server to use to send.
 * @param sockaddr The remote address to send to.
 * @param saddr_len The length of the remote address.
 * @param msg The packet to send.
 * @param msg_len The length of the packet to send.
 * @return The number of bytes sent, or SERVER_ESEND on failure.
 */
static int
_server_send_to(_server* server,
                struct sockaddr* sockaddr,
                size_t saddr_len,
                char* msg,
                size_t msg_len);

/**
 * @brief Utility function that computes the range of the next packet to send.
 *
 * We keep packets below MAX_DGRAM_BUFSIZ in length. This function takes a
 * packet defined by the contiguous memory range between `begin` and `end` and
 * returns a pointer to the end of the first packet. The end of the first packet
 * is defined as the first valid stopping point below begin + MAX_DGRAM_BUFSIZ.
 * In our case, a valid stopping point is a whitespace character.
 *
 * Once a stopping point is found, the whitespace character is replaced with a
 * null terminator, the pointer is advanced one byte, and then returned.
 *
 * @param begin Pointer to the start of the packet.
 * @param end Pointer to one byte past the end of the packet.
 * @return char* Pointer to the byte past the end of the computed chunk.
 */
static char*
_server_next_chunk(char* begin, char* end);

/**
 * @brief Waits for and receives a request.
 *
 * This function will block waiting for a request. When a message is recevied,
 * the request parameter will be filled with the request packet as a string.
 *
 * @param server The server to listen on.
 * @param request A @ref _request to fill with the request.
 * @return SERVER_EOK on succes, SERVER_ERECV on failure.
 */
static int
_server_recv_msg(_server* server, _request* request);

/**
 * @brief Allocates and zeroes a new @ref request_data structure.
 *
 * @return request_data* Pointer to the request_data, or NULL on failure.
 */
static request_data*
_server_new_request_data(void);

/**
 * @brief Allocates and zeroes a new @ref _request structure.
 *
 * Fills out the @ref request_data and @ref socklen_t addr_len members.
 *
 * @return _request* Pointer to the allocated @ref _request or NULL on failure.
 */
static _request*
_server_new_request(void);

/**
 * @brief Destroys and cleans up a request object.
 *
 * @param req The request to destroy.
 */
static void
_server_destroy_request(_request* req);

/**
 * @brief Handles and consumes a request.
 *
 * This function is called from the request handler thread. It calls the
 * server's configured request handler, responds to the sender if necessary, and
 * cleans up the request.
 *
 * @param server The @ref _server that received the request.
 * @param req The @_request to handle.
 * @return SERVER_EOK on success, or a negative error code on failure.
 */
static int
_server_consume_request(_server* server, _request* req);

/**
 * @brief Server request handler thread function.
 *
 * Waits for requests to be enqueued, then dequeues and handles them.
 *
 * @param _arg Thread user argument. This will be the associated server.
 * @return This function returns NULL.
 */
static void*
_server_request_handler(void* _arg);

/**
 * @brief Enqueue a request.
 *
 * @param server The server on which to enqueue the request.
 * @param req The request to enqueue.
 */
static void
_server_enqueue_request(_server* server, _request* req);

/**
 * @brief Server listener thread function.
 *
 * Listens on the server's socket for requests and enqueues them to be
 * picked up by the handler.
 *
 * @param _arg Thread user data, this is the associated server.
 * @return This function returns NULL.
 */
static void*
_server_listener(void* _arg);

static int
_server_open_udp_socket(struct sockaddr* s_in, socklen_t s_len, bool bind_local)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return SERVER_ESOCK;
    }

    if (bind_local && (bind(sock, s_in, s_len) < 0)) {
        return SERVER_EBIND;
    }
    return sock;
}

static int
_server_send_to(_server* server,
                struct sockaddr* sockaddr,
                size_t saddr_len,
                char* msg,
                size_t msg_len)
{

    int status = SERVER_EOK;
    char* begin = msg;
    char* end = msg + msg_len;
    do {
        char* packet_end = _server_next_chunk(begin, end);
        size_t chunk_length = packet_end - begin;
        int status_s =
          sendto(server->sock_fd, begin, chunk_length, 0, sockaddr, saddr_len);
        if (status_s < 0) {
            status = SERVER_ESEND;
            break;
        }
        begin = packet_end;
    } while (begin < end);

    return status;
}

static char*
_server_next_chunk(char* begin, char* end)
{
    // This is a bit of a dangerous one. It assumes we're
    // strictly dealing with plain-text data, and it assumes that that data is
    // separated by whitespace characters (a la the history command output).
    char* packet_end = end;
    if ((end - begin) > 1500) {
        // point to the last char in the packet
        packet_end = begin + 1500 - 1;
        // cycle back until the last space
        while (!isspace(*packet_end)) {
            packet_end--;
        }
        // change the space to be null and packet_end to point to
        // one past the end
        *packet_end = '\0';
        packet_end++;
    }
    return packet_end;
}

static int
_server_recv_msg(_server* server, _request* request)
{
    int bytes_read;
    if ((bytes_read = recvfrom(server->sock_fd,
                               request->data->buffer,
                               MAX_RECV_BUFSIZ - 1,
                               0,
                               (struct sockaddr*)&request->remote_addr,
                               &request->addr_len)) < 0) {
        return SERVER_ERECV;
    }

    request->data->buffer[bytes_read] = '\0';
    request->data->len = bytes_read;

    return SERVER_EOK;
}

static request_data*
_server_new_request_data(void)
{
    request_data* rd = malloc(sizeof(request_data));
    if (rd) {
        memset(rd, 0, sizeof(request_data));
    }
    return rd;
}

static _request*
_server_new_request(void)
{
    _request* req = malloc(sizeof(_request));
    if (!req) {
        return NULL;
    }

    memset(req, 0, sizeof(_request));
    request_data* data = _server_new_request_data();

    if (!data) {
        free(req);
        return NULL;
    }

    req->data = data;
    req->addr_len = sizeof(req->remote_addr);
    return req;
}

static void
_server_destroy_request(_request* req)
{
    free(req->data);
    free(req);
}

static int
_server_consume_request(_server* server, _request* req)
{
    char* response = NULL;
    size_t res_len = 0;

    int handler_status = server->usr_handler.handler(
      req->data, &response, &res_len, server->usr_handler.usr_data);
    int status = SERVER_EOK;

    if (handler_status < 0) {
        fprintf(stderr,
                "SERVER: ERR: Handler fn returned error status %d\n",
                handler_status);
        status = SERVER_EHANDLER;
    } else if (handler_status > 0) {
        status = _server_send_to(server,
                                 (struct sockaddr*)&req->remote_addr,
                                 req->addr_len,
                                 response,
                                 res_len);
        // TODO: We may have only sent part of the response payload.
        // We should update _server_send_to() to return the bytes sent
        // and possibly retry
        free(response);
    } // handler status == 0 means we do not send a response.

    _server_destroy_request(req);
    return status;
}

static void*
_server_request_handler(void* _arg)
{
    _server* server = _arg;

    while (server->status == SERVER_EOK) {
        _request* req;
        pthread_mutex_lock(&server->rq_lock);
        while (server->rq == NULL) {
            pthread_cond_wait(&server->rq_ready, &server->rq_lock);
            // We may have shut down while we were waiting on
            // the condition, so we need to check or else we will
            // deadlock!
            if (server->status != SERVER_EOK) {
                pthread_mutex_unlock(&server->rq_lock);
                return NULL;
            }
        }
        req = server->rq;
        server->rq = req->next;
        pthread_mutex_unlock(&server->rq_lock);

        if (_server_consume_request(server, req) < 0) {
            perror("ERROR: Failed to send");
        }
    }
    return NULL;
}

static void
_server_enqueue_request(_server* server, _request* req)
{
    pthread_mutex_lock(&server->rq_lock);

    req->next = server->rq;
    server->rq = req;

    pthread_mutex_unlock(&server->rq_lock);

    pthread_cond_signal(&server->rq_ready);
}

static void*
_server_listener(void* _arg)
{
    _server* server = _arg;

    struct pollfd sock_poll;
    sock_poll.fd = server->sock_fd;
    sock_poll.events = POLLIN | POLLPRI;

    _request* request = _server_new_request();
    while (server->status == SERVER_EOK) {
        if (!request) {
            fprintf(stderr, "SERVER: Failed to allocate a request!\n");
            // TODO: server->status = SERVER_EALLOC? do we bail?
            // Seems like the kind of thing that is bad and not good.
            continue;
        }

        int poll_result = poll(&sock_poll, 1, 1000);
        if (poll_result > 0) {
            if (_server_recv_msg(server, request) < 0) {
                perror("SERVER: Recv failed");
            } else {
                _server_enqueue_request(server, request);
                request = _server_new_request();
            }
        } else if (poll_result < 0) {
            perror("SERVER: Polling socket failed");
        } // on timeout we simply continue
    }

    shutdown(server->sock_fd, SHUT_RDWR);
    _server_destroy_request(request);
    return NULL;
}

int
server_open(server* server_usr, handler_fn* handler_fn, void* usr_data)
{
    if (server_usr == NULL || handler_fn == NULL) {
        return SERVER_EINVAL;
    }

    _server* server = malloc(sizeof(_server));
    if (!server) {
        return SERVER_EALLOC;
    }
    memset(server, 0, sizeof(_server));

    struct sockaddr_in s_in;
    memset(&s_in, 0, sizeof(s_in));
    s_in.sin_family = AF_INET;
    s_in.sin_port = htons(server_usr->port);
    s_in.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock =
      _server_open_udp_socket((struct sockaddr*)&s_in, sizeof(s_in), true);
    if (sock < 0) {
        free(server);
        return SERVER_ESOCK;
    }

    server->sock_fd = sock;
    server->status = SERVER_EOK;
    server->rq = NULL;
    server->usr_handler.handler = handler_fn;
    server->usr_handler.usr_data = usr_data;

    server_usr->__server = server;

    return SERVER_EOK;
}

int
server_start_listening(server* s)
{
    _server* server = s->__server;
    if (!server) {
        return SERVER_EINVAL;
    }

    pthread_mutex_init(&server->rq_lock, NULL);
    pthread_cond_init(&server->rq_ready, NULL);

    pthread_create(&server->listener, NULL, _server_listener, server);
    pthread_create(
      &server->request_handler, NULL, _server_request_handler, server);

    return SERVER_EOK;
}

void
server_close(server* s)
{
    _server* server = s->__server;
    if (!server) {
        return;
    }

    server->status = SERVER_SHUTDOWN;
    pthread_join(server->listener, NULL);

    // Signal the condition one last time, otherwise a waiting handler thread
    // will just keep waiting.
    pthread_cond_signal(&server->rq_ready);
    pthread_join(server->request_handler, NULL);

    pthread_mutex_destroy(&server->rq_lock);
    pthread_cond_destroy(&server->rq_ready);

    _request* q = server->rq;
    while (q) {
        _request* next = q->next;
        _server_destroy_request(q);
        q = next;
    }

    free(server);
    s->__server = NULL;
}
