// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>

#include "hal/tcp.h"
#include "beatsync.h"

int
main(int argc, char** argv)
{
    if(argc < 2) {
        printf("Need more args\n");
        return 0;
    }

    Tcp_initializeTcpClient(argv[1]);
    BeatSync_initialize();

    while(1) {

    }

    BeatSync_cleanup();
    Tcp_cleanupTcpClient();

    return 0;
}