/**
 * @file tcp.h
 * @brief For TCP server hosting. Interface using observer pattern for separation.
 * Adapted from the slides and from adamtornhill.com for the observer pattern.
*/
#pragma once

#define MAX_LEN 1024
#define PORT 12345
#define EXIT_CODE "TIMETOGOBYE"
#define SEND_FILE "file"
#define MAX_CONNECTIONS 3
#define BEAT_CODE "beat"

// The prototype for the function that is called when an observer is told about a new message
typedef void (*TcpMessageNotification) (void* instance, const char* newMessage, int socketFd);

// A TcpObserver is composed of an instance of any type (to handle storing data related to that observer)
// and a TcpMessageNotification function that is fired when the server gets a message
typedef struct
{
    void* instance;
    TcpMessageNotification notification;

} TcpObserver;

/**
 * Initialize TCP server
*/
void
Tcp_initializeTcpServer();

/**
 * Clean up TCP server (join threads, close sockets)
*/
void
Tcp_cleanUpTcpServer();

/**
 * Send a response to a message from socketFd
 * @param message The response
 * @param socketFd File descriptor of socket to send to.
 * @return ssize_t Return the number of bytes sent or < 0 if fails
*/
ssize_t
Tcp_sendTcpServerResponse(const char* message, int socketFd);

/**
 * Send a file response to a message from socketFd
 * @param path The relative path of the file to send (absolute seems to have issues right now)
 * @param socketFd File descriptor of socket to send to.
 * @return ssize_t Return the number of bytes sent or < 0 if fails
*/
ssize_t
Tcp_sendFile(char* path, int socketFd);

/**
 * Add a new observer
 * @param observer The observer to add
*/
void
Tcp_attachToTcpServer(const TcpObserver* observer);

// TODO: Remove an observer? Not sure if needed really for our usecase

/**
 * Worker thread function for the server. It is not static because there were compilation issues.
*/
void*
tcpServerWorker(void*);