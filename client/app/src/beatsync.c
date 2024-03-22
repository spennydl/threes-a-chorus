#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "beatsync.h"
#include "com/timeutils.h"
#include "hal/tcp.h"
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
    long long nsToSleep;
    long long nsBetweenBeats;
    long long nsDebt = 0;

    while (running) {

        if(nsToSleep > 0) {
            printf("Slept for %lldns but now ...Beat!\n", nsToSleep);
            MidiPlayer_playNextBeat();
        }
        
        long long reqTimeStart = Timeutils_getTimeInNs();
        memset(res, 0, MAX_BUFFER_SIZE);
        Tcp_makeServerRequest(BEAT_CODE, res);
        long long reqTime = Timeutils_getTimeInNs() - reqTimeStart;

        nsToSleep = strtoll(strtok(res, ";"), NULL, 10);
        nsBetweenBeats = strtoll(strtok(NULL, ";"), NULL, 10);
        if (reqTime > 0) {
            nsToSleep -= reqTime;
        }

        if(nsToSleep > 0) {
            long long newDebt = nsDebt - nsToSleep;
            if(newDebt < 0) {
                newDebt = 0;
            }

            nsToSleep -= nsDebt;
            nsDebt = newDebt;
        }

        MidiPlayer_setBpm(60000000000 / nsBetweenBeats);

        if(nsToSleep <= 0)
        {
            nsDebt += -nsToSleep;
            nsToSleep = 0;
        }
        else
        {
            Timeutils_sleepForNs(nsToSleep);
        }
    }

    return NULL;
}
