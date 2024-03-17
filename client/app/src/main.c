// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>


// #include "rfidExample.h"
// #include "tcpExample.h"
// #include "ultrasonicExample.h"
// =======
#include "hal/tcp.h"
#include "beatsync.h"

int
main(int argc, char** argv)
{

    (void)argc;
    (void)argv;

//     return Rfid_example();
// =======
    if(argc < 2) {
        printf("You must specify the server ip (192.168.7.1)\n");
        return 0;
    }

    Tcp_initializeTcpClient(argv[1]);
    BeatSync_initialize();

    while(1) {
        // Just chill man. No way to gracefully shutdown yet :D
    }

    BeatSync_cleanup();
    Tcp_cleanupTcpClient();

    return 0;
}