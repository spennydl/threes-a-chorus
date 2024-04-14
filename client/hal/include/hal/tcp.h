/**
 * @file tcp.h
 * @brief For TCP client handling
 * @author Jet Simon
 */
#pragma once

#include <stdio.h>

#define DEV
#define PORT 12345
#define MAX_BUFFER_SIZE 1024
#define EXIT_CODE "TIMETOGOBYE"
#define SEND_FILE "file"
#define BEAT_CODE "beat"

/**
 * Initialize TCP client from hostname
 * @param hostname Name of the host
 */
void
Tcp_initializeTcpClient(const char* hostname);

/**
 * Cleans up TCP client (joins all threads, closes all sockets)
 */
void
Tcp_cleanupTcpClient();

/**
 * Send a message to the server and expect a response to that message and uses
 * mutex lock to make sure to be thread safe!
 * @param message The message to send
 * @param buffer Buffer to put response into. Expects length to be MAX_SIZE
 * @return ssize_t Return the number of bytes read into buffer or error code if
 * fails
 */
ssize_t
Tcp_makeServerRequest(char* message, char* buffer);

/**
 * Send a message to the server
 * @param message The message to send
 * @return ssize_t Return the number of bytes sent or error code if fails
 */
ssize_t
Tcp_sendMessage(char* message);

/**
 * Block until the server sends a message
 * @param buffer Buffer to put response into. Expects length to be MAX_SIZE
 * @return ssize_t Return the number of bytes read into buffer or error code if
 * fails
 */
ssize_t
Tcp_receiveMessage(char* buffer);

/**
 * Request a file from the server and download it as filename. Uses a mutex lock
 * to be sure to be thread safe.
 * @param message Name to save recieved file as
 * @return Number of bytes downloaded or -1 on fail
 */
ssize_t
Tcp_requestFile(char* fileName);
