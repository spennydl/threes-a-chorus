// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>

#include <udp.h>

// Example function that just sends back what it gets
static void
onMessageRecieved(void* instance, const char* newMessage, struct sockaddr_in sinRemote)
{
    // Not using instance so just casting it to null in this example to get rid of warnings that it isn't used
    // If you know how to make it stop a better way please tell me!
    (void)instance;
    
    Udp_sendUdpServerResponse(newMessage, sinRemote);
}

int
main()
{
    Udp_initializeUdpServer();

    int uselessServerInstance = 42;

    UdpObserver exampleObserver;
    exampleObserver.instance = &uselessServerInstance;
    exampleObserver.notification = onMessageRecieved;

    Udp_attachToUdpServer(&exampleObserver);

    while(true) {
        char input[32];
        printf("Enter anything to stop the server: ");
        scanf("%s", input);
        break;
    }

    Udp_cleanUpUdpServer();
}