// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "das/sequencer.h"
#include "das/fmplayer.h"
#include "hal/rfid.h"
#include "app.h"

#include "melodyGenExample.h"

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
    
    // TODO: Uncomment once sensors are hooked up
    //Singer_run();

    // Does not create a new thread
    App_runApp(argv[1]);

    Sequencer_destroy();
    FmPlayer_close();
    Rfid_shutdown();

    return 0;
}
