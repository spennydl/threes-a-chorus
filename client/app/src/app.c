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

static bool isRunning = false;
static int currentTagId = 0xFF;
static struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };

static void
pollForMidiNote()
{
    int poll_status;
    if ((poll_status = poll(&stdinp, 1, 500)) < 0) {
        // I'm not sure this is likely to happen. If it does we'll
        // simply continue.
        perror("SHUTDOWN: ERR: Polling stdin failed");
        return;
    }
    if (poll_status > 0) {
        if ((stdinp.revents & (POLLIN | POLLPRI)) > 0) {
            // Read the line sent from stdin, otherwise it will get sent
            // to the shell after we exit.
            char buf[128];
            fgets(buf, 128, stdin);
        }
    }
}

static bool
runMidiPlayer(int channel, char* ip)
{
    if (NetMidi_openMidiChannel(ip, channel) < 0) {
        printf("Error: Could not open midi channel\n");
        return false;
    }

    return true;
}

void App_runApp(char* serverIp)
{
    isRunning = true;
    bool midiPlayerIsRunning = false;
    bool onRfid = false;

    while(isRunning) {

        // TODO: Get sensor data here and adjust params

        currentTagId = Rfid_getCurrentTagId();
        onRfid = currentTagId != 0xFF;

        if(onRfid) {
            // If midi player is running poll for note
            if(midiPlayerIsRunning) {
                pollForMidiNote();
            }
            // If not running yet, start midi player
            else {
                // TODO: Shut down melody generation here to prepare for midi player?
                printf("Found tag. Id is %d -> %d\n", currentTagId, currentTagId % 16);
                midiPlayerIsRunning = runMidiPlayer(currentTagId % 16, serverIp); 
            }
        }
        else {
            // If not on tag but player is running, shut down
            if(midiPlayerIsRunning) {
                NetMidi_stop();
                midiPlayerIsRunning = false;
            }
            // If not on tag and player not running, do melody generation
            else {
                // TODO: This is where we put the melody generation stuff
            }
        }
    }
}

void App_shutdownApp()
{
    isRunning = false;
}

bool App_isRunning()
{
    return isRunning;
}