/**
 * @file app.c
 * @brief Main application loop implementation.
 * @author Jet Simon, Louie Lu, Spencer Leslie
 */
#include "app.h"
#include "hal/rfid.h"
#include "netMidiPlayer.h"
#include "singer.h"

#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/** Timeout for the poll() on stdin. */
#define SHUTDOWN_STDINPOLL_TIMEOUT_MS 100
/** Buffer size for the buffer used to consume stdin. */
#define SHUTDOWN_STDINPOLL_BUFSIZ 128

/** Are we currently running? */
static bool isRunning = false;
/** did we receive sigint? */
static volatile sig_atomic_t signalReceived = 0;
/** Holds the last read RFID tag ID. */
static int currentTagId = 0xFF;

/** Subscribes to a midi channel and begins playing from the stream. */
static bool
_runMidiPlayer(int channel, char* ip);

static void
_handleSigint(int sig);

static bool
_runMidiPlayer(int channel, char* ip)
{
    if (NetMidi_openMidiChannel(ip, channel) < 0) {
        fprintf(stderr, "Error: Could not open midi channel\n");
        return false;
    }

    return true;
}

static void
_handleSigint(int sig)
{
    (void)sig; // Only handling SIGINT, so this should be irrelevant.
    signalReceived = 1;
}

static int
_installSigintHandler(void)
{
    struct sigaction sigact;
    // setting SA_RESTART ensures that any interrupted system calls are
    // restarted. This is necessary if the application wants to gracefully
    // finish its current work item before shutting down.
    // TODO this could be configurable, but for this application we'll
    // leave it hardcoded.
    sigact.sa_flags = SA_RESTART;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigact.sa_handler = _handleSigint;

    return sigaction(SIGINT, &sigact, NULL);
}

void
App_runApp(char* serverIp)
{
    int sigStats = _installSigintHandler();
    if (sigStats < 0) {
        fprintf(stderr,
                "WARN: could not install signal handler. Singer will not shut "
                "down gracefully\n");
        perror("signal handler");
    }

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
                fprintf(stderr,
                        "Singer found tag with ID %d -> %d\n",
                        currentTagId,
                        currentTagId % 16);
                midiPlayerIsRunning =
                  _runMidiPlayer(currentTagId % 16, serverIp);

                if (midiPlayerIsRunning) {
                    SegDisplay_setIsSinging(true);
                } else {
                    Singer_sing();
                }
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

        if (signalReceived > 0) {
            isRunning = false;
        }
    }
    fprintf(stderr, "Singer is shutting down\n");
}
