/**
 * @file tcp.h
 * @brief For TCP client handling
*/
#pragma once

#define PORT 12345
#define MAX_BUFFER_SIZE 1024
#define EXIT_CODE "TIMETOGOBYE"

/**
 * Initialize TCP client from hostname
 * @param hostname Name of the host
*/
void
Tcp_initializeTcpClient(char* hostname);

/**
 * Cleans up TCP client (joins all threads, closes all sockets)
*/
void
Tcp_cleanupTcpClient();

/**
 * Send a message to the server and expect a response to that message
 * @param message The message to send
 * @param buffer Buffer to put response into. Expects length to be MAX_SIZE
 * @return ssize_t Return the number of bytes read into buffer or error code if fails
*/
ssize_t
Tcp_makeServerRequest(char* message, char* buffer);