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

    while(running) {
        char res[MAX_BUFFER_SIZE] = {0};
        Tcp_makeServerRequest(BEAT_CODE, res);
        printf("bpm: %s\n", res);
        Timeutils_sleepForMs(1000);
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