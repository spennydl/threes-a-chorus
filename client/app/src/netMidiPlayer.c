/**
 * @file netMidiPlayer.c
 * @brief Implementation of the network midi player.
 * @author Spencer Leslie and Jet Simon
 */
#include "netMidiPlayer.h"

#include "das/fmplayer.h"
#include "hal/tcp.h"
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Format string for the SUB message set to subscribe to a channel. */
#define SUBSCRIBE_TO_CHANNEL_MESSAGE_FMT "SUB %d"

/** Thread handle for the midi player thread. */
static pthread_t _midiPlayerThread;
/** Should we play? */
static int play;

/** A midi event as received from the server. */
typedef struct
{
    int type;
    int eventData;
} MidiEvent;

/** Deserializes a midi event received over the network to a MidiEvent. */
static MidiEvent
_deserializeMidiEvent(const char* message);
/** Sets the current synth voice to the instrument given by the instrument code.
 */
static void
_setInstrumentFromMidiCode(int instrumentCode);
/** Thread worker function. Reads events and plays them as they are received. */
static void*
_playNetMidi(void* _unused);

static MidiEvent
_deserializeMidiEvent(const char* message)
{
    char buffer[MAX_BUFFER_SIZE] = { 0 };
    strncpy(buffer, message, MAX_BUFFER_SIZE);

    MidiEvent event;
    int type = atoi(strtok(buffer, ";"));
    event.type = type;
    int data = atoi(strtok(NULL, ";"));
    event.eventData = data;

    return event;
}

static void
_setInstrumentFromMidiCode(int instrumentCode)
{
    if (instrumentCode <= 8) {
        // Piano
        FmPlayer_setSynthVoice(&FM_DEFAULT_PARAMS);
    } else if (instrumentCode <= 16) {
        // Chromatic Perc.
        FmPlayer_setSynthVoice(&FM_BELL_PARAMS);
    } else if (instrumentCode <= 24) {
        // Organ
        FmPlayer_setSynthVoice(&FM_BIG_PARAMS);
    } else if (instrumentCode <= 32) {
        // Guitar
        FmPlayer_setSynthVoice(&FM_DEFAULT_PARAMS);
    } else if (instrumentCode <= 40) {
        // Bass
        FmPlayer_setSynthVoice(&FM_BASS_PARAMS);
    } else if (instrumentCode <= 48) {
        // Strings
        FmPlayer_setSynthVoice(&FM_DEFAULT_PARAMS);
    } else if (instrumentCode <= 56) {
        // Ensemble
        FmPlayer_setSynthVoice(&FM_AHH_PARAMS);
    } else if (instrumentCode <= 64) {
        // Brass
        FmPlayer_setSynthVoice(&FM_DEFAULT_PARAMS);
    } else if (instrumentCode <= 72) {
        // Reed
        FmPlayer_setSynthVoice(&FM_DEFAULT_PARAMS);
    } else if (instrumentCode <= 80) {
        // Pipe
        FmPlayer_setSynthVoice(&FM_CHIRP_PARAMS);
    } else if (instrumentCode <= 88) {
        // Synth Lead
        FmPlayer_setSynthVoice(&FM_DEFAULT_PARAMS);
    } else if (instrumentCode <= 96) {
        // Synth Pad
        FmPlayer_setSynthVoice(&FM_SHINYDRONE_PARAMS);
    } else if (instrumentCode <= 104) {
        // Synth FX
        FmPlayer_setSynthVoice(&FM_BEEPBOOP_PARAMS);
    } else if (instrumentCode <= 112) {
        // "Ethnic"
        FmPlayer_setSynthVoice(&FM_YOI_PARAMS);
    } else if (instrumentCode <= 120) {
        // Percussive
        FmPlayer_setSynthVoice(&FM_BASS_PARAMS);
    } else if (instrumentCode <= 128) {
        // Sound FX
        FmPlayer_setSynthVoice(&FM_BEEPBOOP_PARAMS);
    } else {
        fprintf(stderr,
                "WARN: Could not turn %d into an instrument!\n",
                instrumentCode);
    }
}

static void*
_playNetMidi(void* _unused)
{
    (void)_unused;

    char requestBuf[MAX_BUFFER_SIZE];
    while (play) {

        ssize_t bytes = Tcp_receiveMessage(requestBuf);
        if (bytes < 0) {
            fprintf(stderr, "WARN: Error receiving message from the server\n");
            perror("Recv error");
            continue;
        }

        if (requestBuf[0] == '\0') {
            continue;
        }

        MidiEvent event = _deserializeMidiEvent(requestBuf);

        if (event.type == MIDIEVENT_NOTE_ON) {
            FmPlayer_setNote(event.eventData - 36);
            FmPlayer_controlNote(NOTE_CTRL_NOTE_ON);
        } else if (event.type == MIDIEVENT_NOTE_OFF) {
            FmPlayer_controlNote(NOTE_CTRL_NOTE_OFF);
        } else if (event.type == MIDIEVENT_PGM_CHANGE) {
            _setInstrumentFromMidiCode(event.eventData);
        } else {
            fprintf(
              stderr, "WARN: Could not parse event type %d\n", event.type);
            continue;
        }
    }

    return NULL;
}

int
NetMidi_openMidiChannel(const char* hostname, NetMidi_Channel channel)
{
    if (Tcp_initializeTcpClient(hostname) == -1) {
        return -1;
    }

    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, MAX_BUFFER_SIZE, SUBSCRIBE_TO_CHANNEL_MESSAGE_FMT, channel);
    ssize_t sent = Tcp_sendMessage(buf);

    if (sent < 0) {
        fprintf(stderr, "Could not subscribe to server\n");
        perror("Subscribe");
        return sent;
    }

    play = 1;

    if (pthread_create(&_midiPlayerThread, NULL, _playNetMidi, NULL) < 0) {
        fprintf(stderr, "Could not start midi player thread\n");
        perror("Midi player thread");
        return -1;
    }

    return 0;
}

void
NetMidi_stop(void)
{
    play = 0;
    pthread_join(_midiPlayerThread, NULL);
    FmPlayer_controlNote(NOTE_CTRL_NOTE_OFF);
    Tcp_cleanupTcpClient();
}
