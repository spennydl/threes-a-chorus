#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>

#include "tcp.h"
#include "beatsync.h"

static int serverInstance = 42;

static void
onMessageRecieved(void* instance, const char* newMessage, int socketFd)
{
    (void)instance;

    if(strcmp(newMessage, BEAT_CODE) == 0) {
        printf("Got: %s. \n", newMessage);
        ssize_t res = Tcp_sendTcpServerResponse("120", socketFd);
        printf("Code from res: %ld\n", res);
    }
}

void
BeatSync_initialize()
{
    TcpObserver exampleObserver;
    exampleObserver.instance = &serverInstance;
    exampleObserver.notification = onMessageRecieved;

    Tcp_attachToTcpServer(&exampleObserver);
}

void BeatSync_cleanup()
{

}