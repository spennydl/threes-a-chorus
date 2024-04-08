#include <poll.h>
#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "com/timeutils.h"
#include "das/melodygen.h"
#include "das/sequencer.h"
#include "hal/rfid.h"
#include "netMidiPlayer.h"
#include "shutdown.h"
#include "singer.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#define POLL_TAG_ID_MS 100
// 10ms
#define NS_TO_WAIT_BETWEEN_UPDATES 10000000

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

static void
_shutdown_handler(int code)
{
    (void)code;
    isRunning = false;
}

void
App_runApp(char* serverIp)
{
    (void)debugMood;

    if (shutdown_install(_shutdown_handler) < 0) {
        fprintf(stderr, "WARN: Failed to start shutdown listener\n");
    }

    isRunning = true;
    bool midiPlayerIsRunning = false;
    bool onRfid = false;

    App_onSequencerLoop();

    while (isRunning) {
        long long start = Timeutils_getTimeInNs();

        currentTagId = Rfid_getCurrentTagId();
        onRfid = currentTagId != 0xFF;

        if (onRfid) {
            if (!midiPlayerIsRunning) {
                // If not running yet, start midi player
                Sequencer_stop();
                printf("Found tag. Id is %d -> %d\n",
                       currentTagId,
                       currentTagId % 16);
                midiPlayerIsRunning =
                  runMidiPlayer(currentTagId % 16, serverIp);
            }
            // otherwise let the midi player play
        } else {
            // If not on tag but player is running, shut down
            if (midiPlayerIsRunning) {
                NetMidi_stop();
                midiPlayerIsRunning = false;
                Sequencer_start();
            }
            Singer_update();
        }
        long long elapsed = Timeutils_getTimeInNs() - start;
        long long toSleep = NS_TO_WAIT_BETWEEN_UPDATES - elapsed;
        Timeutils_sleepForNs(toSleep);
    }
}

void
App_onSequencerLoop()
{
    Sequencer_clear();
    // printf("New sequencer loop\n");
    Mood* mood = Singer_getMood();
    Melody_playMelody(mood);
}

void
App_shutdownApp()
{
    isRunning = false;
}

bool
App_isRunning()
{
    return isRunning;
}
