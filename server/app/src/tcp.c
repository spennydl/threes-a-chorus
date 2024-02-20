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

#include "tcp.h"

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

static atomic_int connections = 0;

// TODO: Turn this into a pool so we can reuse threads!
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
        return;
    }

    if(bind(socketDescriptor, (struct sockaddr*)&tcp_sin, sizeof(tcp_sin)) < 0)
    {
        printf("Error binding to socket during TCP server initialization!\n");
        return;
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
    return send(socketFd, message, strlen(message), 0);
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
tcpServerConnectionHandler(void* socketFdArg)
{
    int socketFd = (int)(uintptr_t)socketFdArg;
    bool connectionOpen = true;

    while(tcpServerRunning && connectionOpen) {
        
        char buffer[MAX_LEN];
        bzero(buffer, MAX_LEN);

        int res = recv(socketFd, buffer, MAX_LEN, 0);
        if (res < 0) {
            printf("Error reading into buffer\n");
            return NULL;
        }

        if (strcmp(buffer, EXIT_CODE) == 0) {
            connectionOpen = false;
            continue;
        }

        if (strlen(buffer) == 0) {
            printf("Closing socket because recieved a message with zero length, this could be bad in the future?\n");
            connectionOpen = false;
            continue;
        }

        sendMessageToObservers(buffer, socketFd);
    }

    close(socketFd);

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

        if(connections > 16) {
            printf("Error: Cannot have more than %d connection threads! Closing incoming connection.\n", MAX_CONNECTIONS);
            close(socketFd);
            continue;
        }

        pthread_create(&connectionThreads[connections], NULL, tcpServerConnectionHandler, (void*)(uintptr_t)socketFd);
        pthread_detach(connectionThreads[connections]);
        connections++;
    }

    return NULL;
}