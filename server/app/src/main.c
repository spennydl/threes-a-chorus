// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <arpa/inet.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "midiPlayer.h"
#include "tcp.h"

int
main()
{
    Tcp_initializeTcpServer();

    MidiPlayer_initialize();

    MidiPlayer_playMidiFile("scale.mid");

    while (1) {
        char input[32];
        printf("Enter exit to quit, file name, orcd  a number for bpm: ");
        scanf("%s", input);

        if (isdigit(input[0])) {
            int newBpm = atoi(input);
            MidiPlayer_setBpm(newBpm);
            printf("Set BPM to %d\n", newBpm);
        }
    }

    /*BeatSync_initialize();
    BeatSync_setMidiToSend("overworld.mid");

    while(true)
    {
        char input[32];
        printf("Enter exit to quit, file name, or a number for bpm: ");
        scanf("%s", input);

        if(isdigit(input[0])) {
            int newBpm = atoi(input);
            BeatSync_setBpm(newBpm);
            printf("Set BPM to %d\n", newBpm);
        }
        else if(strcmp(input, "exit") == 0) {
            break;
        }
        else {
            BeatSync_setMidiToSend(input);
            printf("Set new midi file\n");
        }
    }

    printf("Shutting down!\n");

    BeatSync_cleanup();*/

    MidiPlayer_cleanup();

    Tcp_cleanUpTcpServer();
}