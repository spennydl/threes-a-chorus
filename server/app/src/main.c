// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <tcp.h>
#include <string.h>

// Example function that just sends back what it gets
static void
onMessageRecieved(void* instance, const char* newMessage, int socketFd)
{
    // Not using instance so just casting it to null in this example to get rid of warnings that it isn't used
    // If you know how to make it stop a better way please tell me!
    (void)instance;
    
    printf("Got: %s. Echoing!\n", newMessage);

    ssize_t res = Tcp_sendTcpServerResponse(newMessage, socketFd);
    printf("Code from echo: %ld\n", res);
}

int
main()
{
    Tcp_initializeTcpServer();

    int uselessServerInstance = 42;

    TcpObserver exampleObserver;
    exampleObserver.instance = &uselessServerInstance;
    exampleObserver.notification = onMessageRecieved;

    Tcp_attachToTcpServer(&exampleObserver);

    while(true)
    {
        char input[32];
        printf("Enter anything to stop the server: ");
        scanf("%s", input);
        break;
    }

    printf("Shutting down!\n");

    Tcp_cleanUpTcpServer();
}