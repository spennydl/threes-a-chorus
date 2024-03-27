// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>

#include "beatsync.h"
#include "das/fmplayer.h"
#include "netMidiPlayer.h"
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int
main(int argc, char** argv)
{
    if (argc < 3) {
        printf("You must specify the server ip (192.168.7.1), midi channel");
        return 0;
    }

    FmPlayer_initialize(&FM_DEFAULT_PARAMS);
    int channel = atoi(argv[2]);

    if (NetMidi_openMidiChannel(argv[1], channel) < 0) {
        exit(1);
    }

    struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };
    while (1) {
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
    }

    NetMidi_stop();
    FmPlayer_close();
    return 0;
}
