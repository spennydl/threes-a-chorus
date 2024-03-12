#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#include "hal/timeutils.h"
#include "tcp.h"
#include "beatsync.h"

static int serverInstance = 42;

static pthread_t workerThread;

static bool running = false;

static int bpm = 120;

static atomic_long msLastBeatStarted = 0;

static long
msBetweenBeats()
{
    return (long)(60.0 / bpm * 1000.0);
}

static void
onMessageRecieved(void* instance, const char* newMessage, int socketFd)
{
    (void)instance;

    if(strcmp(newMessage, BEAT_CODE) == 0) {
        char msLeft[MAX_LEN] = {0};
        long msToWaitUntilNextBeat = (msLastBeatStarted + msBetweenBeats()) - Timeutils_getTimeInMs();
        snprintf(msLeft, MAX_LEN - 1, "%ld", msToWaitUntilNextBeat);
        ssize_t res = Tcp_sendTcpServerResponse(msLeft, socketFd);
        (void)res;
    }
}

void
BeatSync_initialize()
{
    TcpObserver exampleObserver;
    exampleObserver.instance = &serverInstance;
    exampleObserver.notification = onMessageRecieved;

    Tcp_attachToTcpServer(&exampleObserver);

    running = true;
    pthread_create(&workerThread, NULL, &beatSyncThreadWorker, NULL);
}

void BeatSync_cleanup()
{
    running = false;
    pthread_join(workerThread, NULL);
}

void* beatSyncThreadWorker(void* p)
{
    (void)p;

    while(running) {
        long msToSleep = msBetweenBeats();
        msLastBeatStarted = Timeutils_getTimeInMs();
        Timeutils_sleepForMs(msToSleep);
        printf("After being eepy for %ldms...Beat!\n", msToSleep);
    }

    return NULL;
}