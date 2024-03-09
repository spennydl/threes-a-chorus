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

static pthread_t beatSyncThread;
static pthread_t beatRequestThread;

void
BeatSync_initialize()
{
    running = true;
    (void)beatSyncWorker;
    (void)beatRequesterWorker;
    pthread_create(&beatRequestThread, NULL, &beatRequesterWorker, NULL);
    pthread_create(&beatSyncThread, NULL, &beatSyncWorker, NULL);
}

void BeatSync_cleanup()
{
    running = false;
    pthread_join(beatSyncThread, NULL);
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
        //printf("ms to sleep: %s > %ld\n", res, msToSleep);
        Timeutils_sleepForMs(msToSleep);
        
        printf("Beat!\n");
    }

    return NULL;
}

void*
beatSyncWorker(void* p)
{
    (void)p;

    while(running) {
        
    }

    return NULL;
}