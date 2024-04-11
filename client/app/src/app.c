#include <poll.h>
#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "hal/rfid.h"
#include "netMidiPlayer.h"
#include "singer.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>

static bool isRunning = false;
static int currentTagId = 0xFF;

static Mood debugMood = { .emotion = EMOTION_HAPPY, .magnitude = 1.0 };

static bool
runMidiPlayer(int channel, char* ip)
{
    if (NetMidi_openMidiChannel(ip, channel) < 0) {
        printf("Error: Could not open midi channel\n");
        return false;
    }

    return true;
}

/** Timeout for the poll() on stdin. */
#define SHUTDOWN_STDINPOLL_TIMEOUT_MS 100
/** Buffer size for the buffer used to consume stdin. */
#define SHUTDOWN_STDINPOLL_BUFSIZ 128

void
App_runApp(char* serverIp)
{
    (void)debugMood;

    isRunning = true;
    bool midiPlayerIsRunning = false;
    bool onRfid = false;

    Singer_sing();

    while (isRunning) {

        currentTagId = Rfid_getCurrentTagId();
        onRfid = currentTagId != 0xFF;

        if (onRfid) {
            if (!midiPlayerIsRunning) {
                Singer_rest();
                printf("Found tag. Id is %d -> %d\n",
                       currentTagId,
                       currentTagId % 16);
                midiPlayerIsRunning =
                  runMidiPlayer(currentTagId % 16, serverIp);
                SegDisplay_setIsSinging(true);
            }
            // otherwise let the midi player play
        } else {
            // If not on tag but player is running, shut down
            if (midiPlayerIsRunning) {
                NetMidi_stop();
                midiPlayerIsRunning = false;
                Singer_sing();
                SegDisplay_setIsSinging(false);
            }
            Singer_update();
        }

        struct pollfd stdinp = { .fd = STDIN_FILENO,
                                 .events = POLLIN | POLLPRI };
        int poll_status;
        if ((poll_status = poll(&stdinp, 1, SHUTDOWN_STDINPOLL_TIMEOUT_MS)) <
            0) {
            // I'm not sure this is likely to happen. If it does we'll simply
            // continue.
            perror("SHUTDOWN: ERR: Polling stdin failed");
        }
        if (poll_status > 0) {
            if ((stdinp.revents & (POLLIN | POLLPRI)) > 0) {
                // Read the line sent from stdin, otherwise it will get sent
                // to the shell after we exit.
                char buf[SHUTDOWN_STDINPOLL_BUFSIZ];
                fgets(buf, SHUTDOWN_STDINPOLL_BUFSIZ, stdin);

                isRunning = false;
            }
        }
    }
}
