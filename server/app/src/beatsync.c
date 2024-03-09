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

static atomic_long msLeftUntilBeat = 0;

static void
onMessageRecieved(void* instance, const char* newMessage, int socketFd)
{
    (void)instance;

    if(strcmp(newMessage, BEAT_CODE) == 0) {
        //printf("Got: %s. \n", newMessage);
        char msLeft[MAX_LEN] = {0};
        snprintf(msLeft, MAX_LEN - 1, "%ld", msLeftUntilBeat);
        ssize_t res = Tcp_sendTcpServerResponse(msLeft, socketFd);
        (void)res;
        //printf("Code from res: %ld\n", res);
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

    (void)bpm;
    long sleepGoal = 500;//(long)(60.0 / bpm * 1000.0);
    int stepSize = 10;

    while(running) {

        for(msLeftUntilBeat = sleepGoal; msLeftUntilBeat > 0; msLeftUntilBeat -= stepSize) {
            Timeutils_sleepForMs(stepSize);
        }

        printf("Beat!\n");
        
    }

    return NULL;
}