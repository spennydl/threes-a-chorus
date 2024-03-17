#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "hal/timeutils.h"
#include "hal/tcp.h"
#include "beatsync.h"
#include "midiPlayer.h"

static bool running = false;

static pthread_t beatRequestThread;

void
BeatSync_initialize()
{
    running = true;
    (void)beatRequesterWorker;
    pthread_create(&beatRequestThread, NULL, &beatRequesterWorker, NULL);
}

void
BeatSync_cleanup()
{
    running = false;
    pthread_join(beatRequestThread, NULL);
}

void
BeatSync_requestBeatOffsetAndMidi()
{
    Tcp_requestFile("test.midi");
    char beatOffsetString[MAX_BUFFER_SIZE] = {0};
    Tcp_makeServerRequest(OFFSET_CODE, beatOffsetString);
    MidiPlayer_setBeatOffset(strtoll(beatOffsetString, NULL, 10));
}

void*
beatRequesterWorker(void* p)
{
    (void)p;
    char res[MAX_BUFFER_SIZE] = {0};
    long msToSleep;
    long msBetweenBeats;

    while(running) {
        memset(res, 0, MAX_BUFFER_SIZE);
        Tcp_makeServerRequest(BEAT_CODE, res);

        msToSleep = strtol(strtok(res, ";"), NULL, 10);
        msBetweenBeats = strtol(strtok(NULL, ";"), NULL, 10);

        MidiPlayer_setBpm(60000 / msBetweenBeats);
        Timeutils_sleepForMs(msToSleep);
        
        //printf("Slept for %ldms but now ...Beat!\n", msToSleep);
        MidiPlayer_playNextBeat();
        
    }

    return NULL;
}