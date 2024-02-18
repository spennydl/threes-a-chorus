#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <hal/udp.h>

static int sockfd;

static unsigned int serverlen;
static struct sockaddr_in serverAddress;

struct hostent *server;

static void
error(char* message)
{
    perror(message);
    exit(0);
}

void
Udp_initializeUdpClient(char* hostname)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("UDP client could not open socket!\n");
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        error("Host does not exist!\n");
    }

    bzero((char*)&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    bcopy((char*)server->h_addr_list[0],
        (char*)&serverAddress.sin_addr.s_addr, server->h_length);
    serverAddress.sin_port = htons(PORT);
    serverlen = sizeof(serverAddress);
}

ssize_t
Udp_makeServerRequest(char* message, char* buffer)
{
    int n = sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&serverAddress, serverlen);
    if (n < 0) {
        error("Send sever to failed!");
    }

    memset(buffer, 0, MAX_BUFFER_SIZE);
    return recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&serverAddress, &serverlen);
}