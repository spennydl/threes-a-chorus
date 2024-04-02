// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "hal/rfid.h"
#include "com/timeutils.h"
#include "das/fmplayer.h"
#include "netMidiPlayer.h"
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#define POLL_TAG_ID_MS 100

static int
runMidiPlayer(int channel, char* ip)
{
    int currentTagId = 0xFF;

    if (NetMidi_openMidiChannel(ip, channel) < 0) {
        printf("Error: Could not open midi channel\n");
        return currentTagId;
    }

    bool onRfid = true;

    struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };
    while (onRfid) {
        int poll_status;
        if ((poll_status = poll(&stdinp, 1, 500)) < 0) {
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

        // Check if on RFID
        currentTagId = Rfid_getCurrentTagId();
        onRfid = currentTagId != 0xFF;
    }

    printf("Disconnecting from server!\n");
    NetMidi_stop();
    return currentTagId;
}

int
main(int argc, char** argv)
{
    if (argc < 2) {
        printf("You must specify the server ip (192.168.7.1)");
        return 0;
    }

    char* ip = argv[1];
    int currentTagId = 0xFF;

    Rfid_init();
    FmPlayer_initialize(&FM_DEFAULT_PARAMS);

    while(1) {
        currentTagId = Rfid_getCurrentTagId();

        if(currentTagId != 0xFF) {
            printf("Found tag. Id is %d -> %d\n", currentTagId, currentTagId % 16);
            currentTagId = runMidiPlayer(currentTagId % 16, ip);
        }
        else {
            Timeutils_sleepForMs(POLL_TAG_ID_MS);
        }
    }

    FmPlayer_close();
    return 0;
}
