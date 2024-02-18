// For TCP server stuff
// Code in this section adapted from the slides
// Observer pattern implementation adapted from adamtornhill.com
#pragma once

#define MAX_LEN 1024
#define PORT 12345
#define EXIT_CODE "TIMETOGOBYE"
#define MAX_CONNECTIONS 16

// The prototype for the function that is called when an observer is told about a new message
typedef void (*TcpMessageNotification) (void* instance, const char* newMessage, int socketFd);

// A TcpObserver is composed of an instance of any type (to handle storing data related to that observer)
// and a TcpMessageNotification function that is fired when the server gets a message
typedef struct
{
    void* instance;
    TcpMessageNotification notification;

} TcpObserver;

void
Tcp_initializeTcpServer();

void
Tcp_cleanUpTcpServer();

// Nice and easy util method for sending back responses!
ssize_t
Tcp_sendTcpServerResponse(const char* message, int socketFd);

// Add a new observer
void
Tcp_attachToTcpServer(const TcpObserver* observer);

// TODO: Remove an observer? Not sure if needed really for our usecase

// Worker thread
void*
tcpServerWorker(void*);