// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <arpa/inet.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "midiPlayer.h"
#include "tcp.h"

int
main()
{
    srand(time(NULL));

    Tcp_initializeTcpServer();

    MidiPlayer_initialize();

    MidiPlayer_playMidiFile("midis/overworld.mid");

    while (1) {
        /*char input[32];
        printf("Enter exit to quit, file name, orcd  a number for bpm: ");
        scanf("%s", input);

        if(strcmp(input, "exit") == 0) {
            break;
        }
        else if(strcmp(input, "shuffle") == 0) {
            char shuffledFile[256];
            MidiPlayer_getRandomMidiPath(shuffledFile);
            printf("Shuffled to MIDI file: %s\n", shuffledFile);
            MidiPlayer_playMidiFile(shuffledFile);
        }
        else if(isdigit(input[0]) || (input[0] == '\n' && isdigit(input[1]))) {
            int newBpm = atoi(input);
            MidiPlayer_setBpm(newBpm);
            printf("Set BPM to %d\n", newBpm);
        }
        else {
            printf("Switching to MIDI file: %s\n", input);
            MidiPlayer_playMidiFile(input);
        }*/
    }

    printf("Shutting down!\n");

    MidiPlayer_cleanup();

    Tcp_cleanUpTcpServer();
}