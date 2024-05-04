// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <poll.h>
#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "hal/rfid.h"
#include "hal/segDisplay.h"
#include "singer.h"

int
main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "You must specify the server ip (192.168.7.1)\n");
        return 0;
    }

    srand(time(NULL));

    Rfid_init();
    Singer_initialize();
    SegDisplay_init();

    // argv[1] is the host ip
    App_runApp(argv[1]);

    SegDisplay_shutdown();
    Singer_shutdown();
    Rfid_shutdown();

    fprintf(stderr, "Singer shut down successfully\n");
    return 0;
}
