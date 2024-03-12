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

typedef struct
{
    SequencerControl ctrl;
    Note note;
} SequencerNote;

#define SEQ_LEN (8 * 4)

const SequencerNote NOISY_SONG[SEQ_LEN] = {
    { .ctrl = SEQ_NOTE_ON, .note = A2 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
};

const SequencerNote BELL_SONG[SEQ_LEN] = {
    { .ctrl = SEQ_NOTE_STOCCATO, .note = A4 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = C5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = D5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = Eb5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = E5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = E5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = G5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = E5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = D5 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = G4 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
};

const SequencerNote BASS_SONG[SEQ_LEN] = {
    { .ctrl = SEQ_NOTE_STOCCATO, .note = A2 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = C3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = D3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = Eb3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = E3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = E3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = G3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = E3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NOTE_STOCCATO, .note = D3 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },

    { .ctrl = SEQ_NOTE_STOCCATO, .note = G2 },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
    { .ctrl = SEQ_NONE, .note = NOTE_NONE },
};

typedef struct
{
    const SequencerNote* notes;
    const FmSynthParams* voice;
    const char* name;
} Song;

static void
_sequence(const SequencerNote* song)
{
    for (int i = 0; i < SEQ_LEN; i++) {
        Sequencer_fillSlot(i, song[i].ctrl, song[i].note, NULL);
    }
}

int
example(void)
{
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

    const Song drones[] = {
        { .notes = NULL, .voice = &FM_BIG_PARAMS, .name = "'Big' drone" },
        { .notes = NULL,
          .voice = &FM_BEEPBOOP_PARAMS,
          .name = "'Beep Boop' drone" },
        { .notes = NULL,
          .voice = &FM_SHINYDRONE_PARAMS,
          .name = "'Shiny' drone" }
    };

    struct pollfd stdinp = { .fd = STDIN_FILENO, .events = POLLIN | POLLPRI };

    size_t msSlept = 0;
    size_t msToSleep = 8000; // 8 sec so seq goes twice
    size_t song_idx = 0;
    size_t n_songs = 7;
    size_t n_drones = 3;
    int quit = 0;

    FmPlayer_initialize(songs[song_idx].voice);

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
            _sleepForMs(1000); // let the note ring out a bit

            _sequence(songs[song_idx].notes);
            FmPlayer_updateSynthParams(songs[song_idx].voice);

            printf("Playing %s\n", songs[song_idx].name);
            Sequencer_reset();

            msSlept = 0;
            song_idx++;
            // Sequencer_start();
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

        _sleepForMs(200); // lets the sequencer go around twice
        msSlept += 200;
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

            FmPlayer_noteOff();
            _sleepForMs(500); // let the note fade
            FmPlayer_updateSynthParams(drones[song_idx].voice);
            printf("Playing %s\n", drones[song_idx].name);

            _sleepForMs(500); // TODO: Sometimes the note doesn't trigger
                              // immediately after updating params?
            FmPlayer_noteOn();

            song_idx++;
            // Sequencer_start();
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

        _sleepForMs(200); // lets the sequencer go around twice
        msSlept += 200;
        if (msSlept >= msToSleep) {
            msSlept = 0;
        }
    }

    FmPlayer_noteOff();

    _sleepForMs(1000); // fadeout!

    Sequencer_destroy();
    FmPlayer_close();
    return 0;
}
