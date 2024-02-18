// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <hal/udp.h>

int 
main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Error: Must specify hostname (ip in this case) of tac server! (ex: ./tac 192.168.7.2)\n");
        return 0;
    }

    Udp_initializeUdpClient(argv[1]);

    char buffer[MAX_BUFFER_SIZE];
    Udp_makeServerRequest("Hello World!", buffer);

    printf("Made request and got: %s\n", buffer);
}