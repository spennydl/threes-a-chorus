#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "beatsync.h"
#include "hal/tcp.h"
#include "hal/timeutils.h"
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
    char beatOffsetString[MAX_BUFFER_SIZE] = { 0 };
    Tcp_makeServerRequest(OFFSET_CODE, beatOffsetString);
    MidiPlayer_setBeatOffset(strtoll(beatOffsetString, NULL, 10));
}

void*
beatRequesterWorker(void* p)
{
    (void)p;
    char res[MAX_BUFFER_SIZE] = { 0 };
    long msToSleep;
    long msBetweenBeats;

    while (running) {
        // printf("Slept for %ldms but now ...Beat!\n", msToSleep);
        MidiPlayer_playNextBeat();

        long long reqTimeStart = Timeutils_getTimeInNs();
        memset(res, 0, MAX_BUFFER_SIZE);
        Tcp_makeServerRequest(BEAT_CODE, res);
        long long reqTime = Timeutils_getTimeInNs() - reqTimeStart;

        msToSleep = strtol(strtok(res, ";"), NULL, 10);
        msBetweenBeats = strtol(strtok(NULL, ";"), NULL, 10);
        if (reqTime > 0) {
            msToSleep -= reqTime;
        }

        MidiPlayer_setBpm(60000 / msBetweenBeats);
        Timeutils_sleepForMs(msToSleep);
    }

    return NULL;
}
