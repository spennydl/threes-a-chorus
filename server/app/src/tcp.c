#include <stdbool.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <fcntl.h>

#include "tcp.h"

struct ConnectionData
{
    int socketFd;
    int connectionIndex;
};

// Linked list of observers so we don't have to do array expansion or anything
struct ListNode
{
    TcpObserver item;
    struct ListNode* next;
};

struct ListNode* head = NULL;

static struct sockaddr_in tcp_sin;
static int socketDescriptor;

pthread_t tcpServerThreadId;
static atomic_bool tcpServerRunning = true;

bool connectionActive[MAX_CONNECTIONS] = {0};
pthread_t connectionThreads[MAX_CONNECTIONS];

static void
sendMessageToObservers(char* message, int socketFd)
{
    struct ListNode* node = head;

    while(node != NULL)
    {
        TcpObserver* observer = &node->item;
        observer->notification(observer->instance, message, socketFd);
        node = node->next;
    }
}

static void
freeObserver(struct ListNode* observer)
{
    if (observer == NULL)
    {
        return;
    }

    freeObserver(observer->next);
    free(observer);
}

static void
freeObservers()
{
    freeObserver(head);
}

void
Tcp_initializeTcpServer()
{
    memset(&tcp_sin, 0, sizeof(tcp_sin));
    tcp_sin.sin_family = AF_INET;
    tcp_sin.sin_addr.s_addr = INADDR_ANY;
    tcp_sin.sin_port = htons(PORT);

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    if(socketDescriptor < 0) 
    {
        printf("Error creating TCP socket during initialization!\n");
        exit(0);
    }

    if(bind(socketDescriptor, (struct sockaddr*)&tcp_sin, sizeof(tcp_sin)) < 0)
    {
        printf("Error binding to socket during TCP server initialization!\n");
        exit(0);
    }

    listen(socketDescriptor, 5);

    pthread_create(&tcpServerThreadId, NULL, tcpServerWorker, NULL);
}

void
Tcp_cleanUpTcpServer()
{
    tcpServerRunning = false;
    
    shutdown(socketDescriptor, SHUT_RDWR);
    pthread_join(tcpServerThreadId, NULL);

    freeObservers();
}

ssize_t
Tcp_sendTcpServerResponse(const char* message, int socketFd)
{
    char msg[MAX_LEN] = {0};
    strncpy(msg, message, strlen(message));
    return send(socketFd, msg, MAX_LEN, MSG_NOSIGNAL);
}

ssize_t
Tcp_sendFile(char* path, int socketFd)
{
    int fd = open(path, O_RDONLY);
    struct stat fileStat;
    char fileSize[MAX_LEN];

    if(fd == -1) {
        char error[512];
        snprintf(error, 512, "Could not open '%s' to send!", path);
        perror(error);
        return fd;
    }

    if(fstat(fd, &fileStat) < 0) {
        perror("Error getting file stat for file to send!");
        return 0;
    }

    snprintf(fileSize, MAX_LEN, "%ld", fileStat.st_size);
    
    // First send file size
    Tcp_sendTcpServerResponse(fileSize, socketFd);

    long int offset = 0;
    int remainingData = fileStat.st_size;
    int sentBytes = -1;

    while(remainingData > 0) {
        sentBytes = sendfile(socketFd, fd, &offset, remainingData < BUFSIZ ? remainingData : BUFSIZ);

        if(sentBytes <= 0) {
            perror("Error while sending file");
            break;
        }

        remainingData -= sentBytes;
        printf("%d bytes sent. %d remaining bytes\n", sentBytes, remainingData);
    }

    close(fd);
    return fileStat.st_size - remainingData;
}

void
Tcp_attachToTcpServer(const TcpObserver* observer)
{
    assert(observer != NULL);

    if (head == NULL)
    {
        struct ListNode* newNode = (struct ListNode*)malloc(sizeof(struct ListNode));
        newNode->item = *observer;
        newNode->next = NULL;
        head = newNode;
        return;
    }

    struct ListNode* node = head;

    while (node->next != NULL)
    {
        node = node->next;
    }

    struct ListNode* newNode = (struct ListNode*)malloc(sizeof(struct ListNode));
    newNode->item = *observer;
    newNode->next = NULL;
    node->next = newNode;
}

static void*
tcpServerConnectionHandler(void* dataPointer)
{
    struct ConnectionData data = *(struct ConnectionData*)dataPointer;

    int socketFd = data.socketFd;
    int connectionIndex = data.connectionIndex;
    bool connectionOpen = true;

    char buffer[MAX_LEN];
    bzero(buffer, MAX_LEN);
    int res = recv(socketFd, buffer, MAX_LEN, 0);
    if (res < 0) {
        printf("Error reading into buffer\n");
        return NULL;
    }

    printf("New connect said: %s\n", buffer);

    sendMessageToObservers(buffer, socketFd);

    while(tcpServerRunning && connectionOpen) {
        int error;
        socklen_t len = sizeof(error);
        int ret = getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &error, &len);

        if(ret != 0 || error != 0) {
            connectionOpen = false;
        }
    }

    close(socketFd);
    connectionActive[connectionIndex] = false;

    return NULL;
}

void*
tcpServerWorker(void* p)
{
    (void)p;
    (void)sendMessageToObservers;

    struct sockaddr_in clientSocket;
    unsigned int clientLength = sizeof(clientSocket);
    int socketFd;

    while (tcpServerRunning) {

        socketFd = accept(socketDescriptor, (struct sockaddr*)&clientSocket, &clientLength);

        if (socketFd < 0) {
            printf("Error accepting socket %d\n", socketFd);
            continue;
        }

        int connectionIndex = -1;
        for(int i = 0; i < MAX_CONNECTIONS; i++) {
            if(!connectionActive[i]) {
                connectionIndex = i;
                break;
            }
        }

        if(connectionIndex == -1) {
            printf("Could not find available thread for connection\n");
            continue;
        }

        struct ConnectionData data;
        data.connectionIndex = connectionIndex;
        data.socketFd = socketFd;

        pthread_create(&connectionThreads[connectionIndex], NULL, tcpServerConnectionHandler, (void*)&data);
        pthread_detach(connectionThreads[connectionIndex]);
    }

    return NULL;
}