// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <poll.h>
#include <stdio.h>

#include "beatsync.h"
#include "com/timeutils.h"
#include "das/fm.h"
#include "das/fmplayer.h"
#include "das/melodygen.h"
#include "das/sequencer.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static MelodyGenParams melodyParams = { .tempo = TEMPO_SLOW,
                                        .key = KEY_MINOR,
                                        .jumpChance = 0.2,
                                        .noteDensity = 0.1,
                                        .upDownTendency = 0.6,
                                        .stoccatoLegatoTendency = 0.2 };

static void
_sequencerLoopCallback(void)
{
    printf("Resetting...\n");
    Sequencer_clear();
    Melody_generateToSequencer(&melodyParams);
}

int
main(void)
{
    srand(time(NULL));
    FmPlayer_initialize(&FM_AHH_PARAMS);

    Sequencer_initialize(220, _sequencerLoopCallback);

    Melody_generateToSequencer(&melodyParams);

    Sequencer_start();

    struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };
    while (1) {
        int poll_status;
        if ((poll_status = poll(&stdinp, 1, 0)) < 0) {
            // I'm not sure this is likely to happen. If it does we'll
            // simply continue.
            perror("SHUTDOWN: ERR: Polling stdin failed");
            continue;
        }
        if (poll_status > 0) {
            if ((stdinp.revents & (POLLIN | POLLPRI)) > 0) {
                // Read the line sent from stdin, otherwise it will get sent
                // to the shell after we exit.
                char buf[128];
                fgets(buf, 128, stdin);

                break;
            }
        }

        Timeutils_sleepForMs(200);
    }

    Sequencer_destroy();
    FmPlayer_close();

    return 0;
}
