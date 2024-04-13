/**
 * @file app.c
 * @brief Main application loop implementation.
 * @author Jet Simon, Louie Lu, Spencer Leslie
 */
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

/** Are we currently running? */
static bool isRunning = false;
/** Holds the last read RFID tag ID. */
static int currentTagId = 0xFF;

/** Subscribes to a midi channel and begins playing from the stream. */
static bool
_runMidiPlayer(int channel, char* ip);

static bool
_runMidiPlayer(int channel, char* ip)
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
                  _runMidiPlayer(currentTagId % 16, serverIp);
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
        Singer_modulateVoice();

        Timeutils_sleepForMs(SHUTDOWN_STDINPOLL_TIMEOUT_MS);
    }
}
