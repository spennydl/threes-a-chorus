/**
 * @file server.h
 * @brief Simple UDP server.
 *
 * Listens on a configurable port for plain-text UDP packets. When a packet is
 * received, it's passed to a callback to handle and optionally return data to
 * send back to the sender.
 *
 * The server should first be initialized with a call to @ref server_open which
 * will acquire a socket and relevant resources. A server can then be made to
 * start listening by calling @ref server_start_listening.
 * @ref server_close will tear down a server.
 *
 * @author Spencer Leslie 301571329
 */
#ifndef SPENNYDL_SERVER_H
#define SPENNYDL_SERVER_H
#include <stddef.h>
#include <stdint.h>

/** Status indicating OK. */
#define SERVER_EOK 1
/** Status indicating server shutdown. */
#define SERVER_SHUTDOWN 0
/** Error code indicating a failure in opening a socket. */
#define SERVER_ESOCK -1
/** Error code indicating an allocation failure. */
#define SERVER_EALLOC -2
/** Error code indicating an error binding to a socket. */
#define SERVER_EBIND -3
/** Error codee indicating a failed `recv` call. */
#define SERVER_ERECV -4
/** Error code indicating that a handler callback returned an error code. */
#define SERVER_EHANDLER -5
/** Error code indicating that a send operation failed. */
#define SERVER_ESEND -6
/** Error code indicating an invalid argument. */
#define SERVER_EINVAL -7

/**
 * Maximum size of a receive buffer. This is chosen to be approximately the MTU
 * of most networks, at least as far as I can find.
 */
#define MAX_RECV_BUFSIZ 1500

/**
 * A handle to a server.
 *
 * The port field should be filled out with the port to listen on before passing
 * to @ref server_open.
 */
typedef struct
{
    /** The port on which to listen. */
    uint16_t port;
    /** Internal server pointer. */
    void* __server;
} server;

/**
 * Data received in a request.
 */
typedef struct
{
    /** Packet contents as a string. */
    char buffer[MAX_RECV_BUFSIZ];
    /** Length of the packet contents.*/
    size_t len;
} request_data;

/** Status code indicating handler was OK and wants to respond to the sender. */
#define HANDLER_OK_RESPOND 1
/**
 * Status code indicating handler was OK and does not want to respond to the
 * sender.
 */
#define HANDLER_OK_NORESPONSE 0

/**
 * @brief Type of a request handler function.
 *
 * The server will pass the request packet as req_data. This function should
 * parse the request and allocate and fill a response. This function
 * should update the response_data parameter (which will initially be NULL) to
 * point at the pointer to the response and update the response_len parameter
 * to be the length of the response data in bytes.
 *
 * The server will take ownership of the data pointed to by response_data and
 * will free it once the response has been sent. Implementors of handler
 * functions should _not_ free the memory!
 *
 * The handler function should return HANDLER_OK_RESPOND to request the data be
 * sent back as a response, and HANDLER_OK_NORESPOND to skip responding.
 * In the case where HANDLER_OK_NORESPOND is returned, it is assumed that
 * nothing was written to response_data and the server will _not_ attempt to
 * free it.
 *
 * In case of an error, this function should return a negative number. This will
 * cause the server to abort the rest of the request handling and log the
 * failure.
 *
 * The usr_data parameter is a void pointer to arbitrary data required by the
 * handler. The server will set this parameter to be whatever was passed in to
 * the usr_data parameter of @ref server_open.
 */
typedef int
handler_fn(request_data* req_data,
           char** response_data,
           size_t* response_len,
           void* usr_data);

/**
 * @brief Initializes the server and acquires necessary resources.
 *
 * Acquires a socket on the port set in the @ref server struct,
 * and perform other initialization required.
 *
 * @param server The @ref server to initialize. The `port` member should be set
 * to the port to listen on.
 * @param handler The handler callback to invoke when requests are received.
 * @param user_data A void pointer to some user data required by the handler.
 * Whatever is passed in here will be given to the handler function in the
 * usr_data argument.
 *
 * @return int SERVER_EOK on success, or one of the other error codes on
 * failure.
 */
int
server_open(server* server, handler_fn handler, void* usr_data);

/**
 * @brief Starts a server listening.
 *
 * @param server The server to start listening.
 * @return int Status of the operation. SERVER_EOK on success, or a negative
 * error code on failure.
 */
int
server_start_listening(server* server);

/**
 * @brief Stops the server.
 *
 * Any requests that have been dequeued will be completed before shutdown. All
 * currently queued requests will be dropped.
 *
 * @param server The server to shut down.
 */
void
server_close(server* server);

#endif // SPENNYDL_SERVER_H
