// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "das/fmplayer.h"
#include "das/sequencer.h"
#include "hal/rfid.h"
#include "singer.h"
#include "hal/segDisplay.h"

int
main(int argc, char** argv)
{
    if (argc < 2) {
        printf("You must specify the server ip (192.168.7.1)");
        return 0;
    }

    Rfid_init();

    Sequencer_initialize(220, App_onSequencerLoop);
    FmPlayer_initialize(&FM_DEFAULT_PARAMS);
    srand(time(NULL));
    Singer_initialize();
    SegDisplay_init();

    // Does not create a new thread
    App_runApp(argv[1]);

    SegDisplay_shutdown();
    Singer_shutdown();
    Sequencer_destroy();
    FmPlayer_close();
    Rfid_shutdown();

    printf("\n");
    return 0;
}
