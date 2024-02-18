/**
 * @file udp.h
 * @brief For UDP server stuff
 * Code in this section adapted from the slides
 * Observer pattern implementation adapted from adamtornhill.com
*/

#pragma once

#define MAX_LEN 1024
#define PORT 12345

// The prototype for the function that is called when an observer is told about a new message
typedef void (*UdpMessageNotification) (void* instance, const char* newMessage, struct sockaddr_in sinRemote);

// A UdpObserver is composed of an instance of any type (to handle storing data related to that observer)
// and a UdpMessageNotification function that is fired when the server gets a message
typedef struct
{
    void* instance;
    UdpMessageNotification notification;

} UdpObserver;

/**
 * Initialize UDP server module
*/
void
Udp_initializeUdpServer();

/**
 * Clean up UDP server module
*/
void
Udp_cleanUpUdpServer();


/**
 * Easy way to send response to a message given:
 * @param message The message to send
 * @param sinRemote Address of reciever
 * @return ssize_t Code of send operation (< 0 means an error!)
*/
ssize_t
Udp_sendUdpServerResponse(const char* message, struct sockaddr_in sinRemote);

/**
 * Attach and observer to the UDP module
 * @param observer The observer to add
*/
void
Udp_attachToUdpServer(const UdpObserver* observer);

// TODO: Remove an observer? Not sure if needed really for our usecase

/**
 * Worker thread
*/
void*
udpServerWorker(void*);