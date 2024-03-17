// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>

#include "hal/tcp.h"
#include "beatsync.h"
#include "midiPlayer.h"
#include "das/fmplayer.h"

int
main(int argc, char** argv)
{
    if(argc < 3) {
        printf("You must specify the server ip (192.168.7.1), midi channel");
        return 0;
    }

    int channel = atoi(argv[2]);

    Tcp_initializeTcpClient(argv[1]);
    FmPlayer_initialize(&FM_DEFAULT_PARAMS);

    MidiPlayer_initialize();
    BeatSync_initialize();
    BeatSync_requestBeatOffsetAndMidi();

    MidiPlayer_playMidiFile("test.midi", channel);

    while(1) {
        // Just chill man. No way to gracefully shutdown yet :D
    }

    MidiPlayer_cleanup();
    FmPlayer_close();
    BeatSync_cleanup();
    Tcp_cleanupTcpClient();
    return 0;
}