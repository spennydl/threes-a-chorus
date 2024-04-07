// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "das/fmplayer.h"
#include "hal/rfid.h"

int
main(int argc, char** argv)
{
    if (argc < 2) {
        printf("You must specify the server ip (192.168.7.1)");
        return 0;
    }

    Rfid_init();
    FmPlayer_initialize(&FM_DEFAULT_PARAMS);

    // Does not create a new thread
    App_runApp(argv[1]);

    FmPlayer_close();
    Rfid_shutdown();

    return 0;
}
