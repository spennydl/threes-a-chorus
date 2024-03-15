// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>

#include "hal/tcp.h"
#include "beatsync.h"
#include "midiPlayer.h"
#include "das/fmplayer.h"

int
main()
{
    FmPlayer_initialize(&FM_DEFAULT_PARAMS);
    MidiPlayer_initialize();

    MidiPlayer_playMidiFile("stillalive.midi");
    while(1)
    {

    }

    MidiPlayer_cleanup();
    FmPlayer_close();
/*
    if(argc < 2) {
        printf("You must specify the server ip (192.168.7.1)\n");
        return 0;
    }

    Tcp_initializeTcpClient(argv[1]);
    BeatSync_initialize();

    while(1) {
        // Just chill man. No way to gracefully shutdown yet :D
    }

    BeatSync_cleanup();
    Tcp_cleanupTcpClient();
*/
    return 0;
}