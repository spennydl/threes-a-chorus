/**
 * @file tcpExample.h
 * @brief Just contains an example function for TCP
*/

#include "hal/tcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <hal/tcp.h>
#include <string.h>

/**
 * Just an example function that takes in main's args to use them as main would
*/
void
tcpExample(int argc, char **argv)
{
    if (argc != 2) {
        printf("Error: Must specify hostname (ip in this case) of tac server! (ex: ./tac 192.168.7.1)\n");
        exit(0);
    }

    Tcp_initializeTcpClient(argv[1]);

    char buffer[MAX_BUFFER_SIZE];

    while(1) {
        char message[128];
        printf("Enter message (stop to stop): ");
        scanf("%s", message);

        if(strcmp(message, "stop") == 0) {
            break;
        }

        ssize_t res = Tcp_makeServerRequest(message, buffer);
        printf("Made request and got (%d): %s\n", res, buffer);
    }

    Tcp_cleanupTcpClient();
}