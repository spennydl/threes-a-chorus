#include "netMidiPlayer.h"

#include "das/fmplayer.h"
#include "hal/tcp.h"
#include "lib/midi-parser.h"
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUBSCRIBE_TO_CHANNEL_MESSAGE_FMT "SUB %d"

static pthread_t _midiPlayerThread;
static int play;

typedef struct
{
    int type;
    int eventData;
} MidiEvent;

static MidiEvent
_deserializeMidiEvent(const char* message)
{
    MidiEvent event;
    int type = atoi(message);
    event.type = ntohl(type);

    int i;
    for (i = 0; i < MAX_BUFFER_SIZE && message[i] != '\0'; i++) {
        if (message[i] == ';') {
            i += i;
            break;
        }
    }
    int data = atoi(message + i);
    event.eventData = ntohl(data);

    return event;
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

        MidiEvent event = _deserializeMidiEvent(requestBuf);

        if (event.type == MIDI_STATUS_NOTE_ON) {
            FmPlayer_setNote(event.eventData - 36);
            FmPlayer_controlNote(NOTE_CTRL_NOTE_ON);
        } else if (event.type == MIDI_STATUS_NOTE_OFF) {
            FmPlayer_controlNote(NOTE_CTRL_NOTE_ON);
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
    Tcp_initializeTcpClient(hostname);

    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, MAX_BUFFER_SIZE, SUBSCRIBE_TO_CHANNEL_MESSAGE_FMT, channel);
    ssize_t sent = Tcp_sendMessage(buf);

    if (sent < 0) {
        fprintf(stderr, "Could not subscribe to server\n");
        perror("Subscribe");
        return sent;
    }

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
}
