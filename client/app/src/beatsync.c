#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "hal/timeutils.h"
#include "hal/tcp.h"
#include "beatsync.h"

static bool running = false;

static pthread_t beatRequestThread;

void
BeatSync_initialize()
{
    running = true;
    (void)beatRequesterWorker;
    pthread_create(&beatRequestThread, NULL, &beatRequesterWorker, NULL);
}

void BeatSync_cleanup()
{
    running = false;
    pthread_join(beatRequestThread, NULL);
}

void*
beatRequesterWorker(void* p)
{
    (void)p;
    char res[MAX_BUFFER_SIZE] = {0};
    long msToSleep;

    while(running) {
        memset(res, 0, MAX_BUFFER_SIZE);
        Tcp_makeServerRequest(BEAT_CODE, res);
        msToSleep = strtol(res, NULL, 10);
        Timeutils_sleepForMs(msToSleep);
        
        printf("Slept for %ldms but now ...Beat!\n", msToSleep);
        // TODO make sound here. If we need granularity of more than a quarter note
        // We should do those calculations locally I think...
    }

    return NULL;
}