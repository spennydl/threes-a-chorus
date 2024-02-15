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

#include "udp.h"

// Linked list of observers so we don't have to do array expansion or anything
struct ListNode
{
    UdpObserver item;
    struct ListNode* next;
};

struct ListNode* head = NULL;

static struct sockaddr_in udp_sin;
static int socketDescriptor;

pthread_t udpServerThreadId;
static atomic_bool udpServerRunning = true;

static void sendMessageToObservers(char* message, struct sockaddr_in sinRemote)
{
    struct ListNode* node = head;

    while(node != NULL)
    {
        UdpObserver* observer = &node->item;
        observer->notification(observer->instance, message, sinRemote);
        node = node->next;
    }
}

static void freeObserver(struct ListNode* observer)
{
    if (observer == NULL)
    {
        return;
    }

    freeObserver(observer->next);
    free(observer);
}

static void freeObservers()
{
    freeObserver(head);
}

void Udp_initializeUdpServer()
{
    memset(&udp_sin, 0, sizeof(udp_sin));
    udp_sin.sin_family = AF_INET;
    udp_sin.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_sin.sin_port = htons(PORT);

    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

    if(socketDescriptor == -1) 
    {
        printf("Error creating UDP socket during initialization!\n");
    }

    if(bind(socketDescriptor, (struct sockaddr*)&udp_sin, sizeof(udp_sin)) == -1)
    {
        printf("Error binding to socket during UDP server initialization!\n");
    }

    pthread_create(&udpServerThreadId, NULL, udpServerWorker, NULL);
}

void Udp_cleanUpUdpServer()
{
    udpServerRunning = false;
    
    shutdown(socketDescriptor, SHUT_RDWR);
    pthread_join(udpServerThreadId, NULL);

    freeObservers();
}

ssize_t Udp_sendUdpServerResponse(const char* message, struct sockaddr_in sinRemote)
{
    unsigned int sin_len = sizeof(sinRemote);
    return sendto(socketDescriptor, message, strlen(message), 0, (struct sockaddr*)&sinRemote, sin_len);
}

void Udp_attachToUdpServer(const UdpObserver* observer)
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

void* udpServerWorker(void* p)
{
    (void)p;

    while(udpServerRunning)
    {
        struct sockaddr_in sinRemote;
        unsigned int sin_len = sizeof(sinRemote);
        char messageRx[MAX_LEN];

        int bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN - 1,
        0, (struct sockaddr*)&sinRemote, &sin_len);

        if(bytesRx < 0)
        {
            char address[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sinRemote.sin_addr, address, sizeof(address));
            printf("UDP server could not read message from %s\n", address);
        }

        // Make sure string null terminates
        messageRx[bytesRx] = 0;

        sendMessageToObservers(messageRx, sinRemote);
    }

    return NULL;
}