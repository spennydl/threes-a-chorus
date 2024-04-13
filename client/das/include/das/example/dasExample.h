/**
 * @file dasExample.h
 * @brief An example showcasing the use of the FmSynthesizer and sequencer.
 * @author Spencer Leslie.
 */
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

/** A note for the sequencer. Used to define a song. */
typedef struct
{
    FmPlayer_NoteCtrl ctrl;
    Note note;
} SequencerNote;

#define SEQ_LEN (8 * 4)

/* Songs for the sequencer to play. */

const SequencerNote NOISY_SONG[SEQ_LEN] = {
    { .ctrl = NOTE_CTRL_NOTE_ON, .note = A2 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
};

const SequencerNote BELL_SONG[SEQ_LEN] = {
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = A4 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = C5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = D5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = Eb5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = E5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = E5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = G5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = E5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = D5 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = G4 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
};

const SequencerNote BASS_SONG[SEQ_LEN] = {
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = A2 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = C3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = D3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = Eb3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = E3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = E3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = G3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = E3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = D3 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },

    { .ctrl = NOTE_CTRL_NOTE_STOCCATO, .note = G2 },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
    { .ctrl = NOTE_CTRL_NONE, .note = NOTE_NONE },
};

/** A song. */
typedef struct
{
    const SequencerNote* notes;
    const FmSynthParams* voice;
    const char* name;
} Song;

/** Sends a song to the sequencer. */
static void
_sequence(const SequencerNote* song)
{
    for (int i = 0; i < SEQ_LEN; i++) {
        Sequencer_fillSlot(i, song[i].ctrl, song[i].note, NULL);
    }
}

/** The example. */
int
example(void)
{
    // Songs to play and the voices to use to play them.
    const Song songs[] = {
        { .notes = BELL_SONG,
          .voice = &FM_DEFAULT_PARAMS,
          .name = "Default voice" },
        { .notes = BASS_SONG, .voice = &FM_BASS_PARAMS, .name = "Bass voice" },
        { .notes = BASS_SONG, .voice = &FM_AHH_PARAMS, .name = "'AH' voice" },
        { .notes = BELL_SONG, .voice = &FM_BELL_PARAMS, .name = "Bell voice" },
        { .notes = BELL_SONG,
          .voice = &FM_CHIME_PARAMS,
          .name = "Chime voice" },
        { .notes = BELL_SONG, .voice = &FM_YOI_PARAMS, .name = "'Yoi' voice" },
        { .notes = BELL_SONG,
          .voice = &FM_CHIRP_PARAMS,
          .name = "Chirp voice" },
    };

    // Drones to play and the voices to use to play them.
    const Song drones[] = {
        { .notes = NULL, .voice = &FM_BIG_PARAMS, .name = "'Big' drone" },
        { .notes = NULL,
          .voice = &FM_BEEPBOOP_PARAMS,
          .name = "'Beep Boop' drone" },
        { .notes = NULL,
          .voice = &FM_SHINYDRONE_PARAMS,
          .name = "'Shiny' drone" }
    };

    // Used to poll stdin so that pressing enter exits.
    struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };

    size_t msSlept = 0;
    size_t msToSleep = 8000; // 8 sec so seq goes twice
    size_t song_idx = 0;
    size_t n_songs = 7;
    size_t n_drones = 3;
    int quit = 0;
    double voltReading = 0;
    double op0Cm = 1.0;

    // Start the FmPlayer. This will also initialized the FmSynthesizer.
    FmPlayer_initialize(songs[song_idx].voice);

    // Initialize the sequencer.
    Sequencer_initialize(120);

    //------------------
    // Play Songs
    //------------------
    printf("PLAYING 'MELODIC' VOICES\n");
    while (!quit) {
        if (!msSlept) {
            if (song_idx >= n_songs) {
                break;
            }

            Sequencer_stop();

            _sleepForMs(1000); // let the last note ring out a bit

            _sequence(songs[song_idx].notes);
            FmPlayer_setSynthVoice(songs[song_idx].voice);

            printf("Playing %s\n", songs[song_idx].name);
            Sequencer_reset();

            op0Cm = songs[song_idx].voice->opParams[FM_OPERATOR0].CmRatio;
            msSlept = 0;
            song_idx++;
        }

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
                quit = 1;
                break;
            }
        }

        // Read the on-board potentiometer and use it to modulate the synth
        // voice.
        double newAdcReading =
          ((double)adc_voltage_raw(ADC_CHANNEL0) / ADC_MAX_READING) * 10;
        // Snap the reading at the midpoint
        if (newAdcReading >= 4.8 && newAdcReading <= 5.2) {
            newAdcReading = 5;
        }

        if (op0Cm > 0 && newAdcReading != voltReading) {
            // 'Tune' the CM either up or down
            if (newAdcReading < 5 && newAdcReading >= 0) {
                FmPlayer_updateOperatorCm(FM_OPERATOR0,
                                          op0Cm / (5 - newAdcReading));
            } else if (newAdcReading >= 5) {
                FmPlayer_updateOperatorCm(FM_OPERATOR0,
                                          op0Cm + (newAdcReading - 5));
            }
            voltReading = newAdcReading;
        }

        // Lets the sequencer go around twice
        _sleepForMs(100);
        msSlept += 100;
        if (msSlept >= msToSleep) {
            msSlept = 0;
        }
    }

    Sequencer_stop();
    msSlept = 0;
    song_idx = 0;

    //------------------
    // Play Drones
    //------------------
    printf("PLAYING 'DRONE' VOICES\n");
    FmPlayer_setNote(C4);

    while (!quit) {
        if (!msSlept) {
            if (song_idx >= n_drones) {
                break;
            }

            FmPlayer_controlNote(NOTE_CTRL_NOTE_OFF);
            _sleepForMs(1000); // let the note fade

            FmPlayer_setSynthVoice(drones[song_idx].voice);
            printf("Playing %s\n", drones[song_idx].name);
            FmPlayer_controlNote(NOTE_CTRL_NOTE_ON);

            op0Cm = drones[song_idx].voice->opParams[FM_OPERATOR0].CmRatio;
            song_idx++;
        }

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
                quit = 1;
                break;
            }
        }

        double newAdcReading =
          ((double)adc_voltage_raw(ADC_CHANNEL0) / ADC_MAX_READING) * 10;
        // The reading is noisy, make sure we can get back to zero
        if (newAdcReading >= 4.8 && newAdcReading <= 5.2) {
            newAdcReading = 5;
        }

        if (op0Cm > 0 && newAdcReading != voltReading) {
            if (newAdcReading < 5) {
                FmPlayer_updateOperatorCm(FM_OPERATOR0,
                                          op0Cm / (5 - newAdcReading));
            } else if (newAdcReading >= 5) {
                FmPlayer_updateOperatorCm(FM_OPERATOR0,
                                          op0Cm + (newAdcReading - 5));
            }
            voltReading = newAdcReading;
        }
        _sleepForMs(100); // lets the sequencer go around twice
        msSlept += 100;
        if (msSlept >= msToSleep) {
            msSlept = 0;
        }
    }

    FmPlayer_controlNote(NOTE_CTRL_NOTE_OFF);

    _sleepForMs(1000); // fadeout!

    Sequencer_destroy();
    FmPlayer_close();
    return 0;
}
