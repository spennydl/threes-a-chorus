#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <hal/tcp.h>

static int sockfd;

static unsigned int serverlen;
static struct sockaddr_in serverAddress;

static struct hostent *server;

static ssize_t
Tcp_sendExitCode()
{
    return send(sockfd, EXIT_CODE, strlen(EXIT_CODE), 0);
}

static void
error(char* message)
{
    perror(message);
    exit(0);
}

void
Tcp_initializeTcpClient(char* hostname)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("TCP client could not open socket!\n");
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        error("Host does not exist!\n");
    }

    bzero((char*)&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(hostname);
    serverAddress.sin_port = htons(PORT);
    serverlen = sizeof(serverAddress);

    if (connect(sockfd, (struct sockaddr*)&serverAddress, serverlen) != 0) {
        error("Error connecting to TCP server!\n");
        return;
    }
}

void
Tcp_cleanupTcpClient()
{
    Tcp_sendExitCode();
    close(sockfd);
}

ssize_t
Tcp_makeServerRequest(char* message, char* buffer)
{
    send(sockfd, message, strlen(message), 0);
    bzero(buffer, MAX_BUFFER_SIZE);
    return recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
}