#pragma once

// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include "das/fm.h"
#include "das/fmplayer.h"
#include "das/sequencer.h"
#include "hal/adc.h"
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void
_sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = { seconds, nanoseconds };
    nanosleep(&reqDelay, (struct timespec*)NULL);
}

int
example(void)
{
    FmSynthParams params;
    memcpy(&params, &FM_BASS_PARAMS, sizeof(FmSynthParams));

    FmPlayer_initialize(&params);

    Sequencer_initialize(120);

    Sequencer_fillSlot(Sequencer_getSlotIndex(0, 0, 0), SEQ_NOTE_ON, A4, NULL);
    Sequencer_fillSlot(
      Sequencer_getSlotIndex(0, 1, 0), SEQ_NOTE_OFF, NOTE_NONE, NULL);

    Sequencer_fillSlot(Sequencer_getSlotIndex(1, 0, 0), SEQ_NOTE_ON, C4, NULL);
    Sequencer_fillSlot(
      Sequencer_getSlotIndex(1, 1, 0), SEQ_NOTE_OFF, NOTE_NONE, NULL);

    Sequencer_fillSlot(Sequencer_getSlotIndex(2, 1, 0), SEQ_NOTE_ON, D4, NULL);
    Sequencer_fillSlot(
      Sequencer_getSlotIndex(3, 0, 0), SEQ_NOTE_LEGATO, Eb4, NULL);
    Sequencer_fillSlot(
      Sequencer_getSlotIndex(3, 1, 0), SEQ_NOTE_LEGATO, E4, NULL);

    Sequencer_fillSlot(
      Sequencer_getSlotIndex(4, 1, 0), SEQ_NOTE_LEGATO, E4, NULL);
    Sequencer_fillSlot(
      Sequencer_getSlotIndex(5, 1, 0), SEQ_NOTE_LEGATO, G4, NULL);

    Sequencer_start();

    struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };
    // double reading = 0;
    while (true) {

        // poll for input on STDIN
        // This just allows pressing enter to quit the program.
        int poll_status;
        if ((poll_status = poll(&stdinp, 1, 0)) < 0) {
            // I'm not sure this is likely to happen. If it does we'll simply
            // continue.
            perror("SHUTDOWN: ERR: Polling stdin failed");
        }
        if (poll_status > 0) {
            if ((stdinp.revents & (POLLIN | POLLPRI)) > 0) {
                // Read the line sent from stdin, otherwise it will get sent
                // to the shell after we exit.
                char buf[64];
                fgets(buf, 64, stdin);
                break;
            }
        }

        // Read a value from the pot
        // double newReading =
        //(double)adc_voltage_raw(ADC_CHANNEL0) / ADC_MAX_READING;
        //
        //// If we have a new reading then update the params.
        // if (newReading != reading) {
        // reading = newReading;
        // params.opParams[FM_OPERATOR1].algorithmConnections[FM_OPERATOR2] =
        // reading;
        // FmPlayer_updateSynthParams(&params);
        //}
        _sleepForMs(100); // do it 10 times a sec
    }

    Sequencer_stop();

    _sleepForMs(1000); // fadeout!

    Sequencer_destroy();
    FmPlayer_close();
    return 0;
}
