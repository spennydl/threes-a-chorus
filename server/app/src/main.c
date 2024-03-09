// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>

#include "tcp.h"
#include "beatsync.h"

int
main()
{
    Tcp_initializeTcpServer();
    BeatSync_initialize();

    while(true)
    {
        char input[32];
        printf("Enter anything to stop the server: ");
        scanf("%s", input);
        break;
    }

    printf("Shutting down!\n");

    BeatSync_cleanup();
    Tcp_cleanUpTcpServer();
}