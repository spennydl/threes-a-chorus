// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "hal/segDisplay.h"
#include "hal/timeutils.h"
#include "midiPlayer.h"
#include "tcp.h"

static sig_atomic_t sigintRecvd = 0;

static int
_installSigintHandler(void);
static void
_handleSigint(int sig);

static int
_installSigintHandler(void)
{
    struct sigaction sigact;
    sigact.sa_flags = SA_RESTART;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigact.sa_handler = _handleSigint;

    return sigaction(SIGINT, &sigact, NULL);
}

static void
_handleSigint(int _sig)
{
    (void)_sig;
    sigintRecvd = 1;
}

int
main()
{
    if (_installSigintHandler() < 0) {
        fprintf(stderr,
                "WARN: Could not install sigint handler. The server will not "
                "shut down gracefully\n");
        perror("sigint handler");
    }

    srand(time(NULL));

    SegDisplay_init();

    if (Tcp_initializeTcpServer() != SERVER_OK) {
        perror("Could not intitialize");
        SegDisplay_displayStatus(SERVER_ERROR);
        exit(SERVER_ERROR);
    }

    if (MidiPlayer_initialize() != SERVER_OK) {
        SegDisplay_displayStatus(SERVER_ERROR);
        perror("Could not start the midi player");
        Tcp_cleanUpTcpServer();
        exit(SERVER_ERROR);
    }

    if (MidiPlayer_playMidiFile("midis/overworld.mid") != SERVER_OK) {
        SegDisplay_displayStatus(SERVER_ERROR);
        perror("Could not play overworld.mid");
        MidiPlayer_cleanup();
        Tcp_cleanUpTcpServer();
        exit(SERVER_ERROR);
    }

    // If server starts successfully, display success code on seg display.
    SegDisplay_displayStatus(SERVER_OK);

    while (sigintRecvd == 0) {
        Timeutils_sleepForMs(500);
    }

    MidiPlayer_cleanup();
    Tcp_cleanUpTcpServer();
    SegDisplay_shutdown();
}
