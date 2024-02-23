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
    Tcp_sendMessage(message);
    bzero(buffer, MAX_BUFFER_SIZE);
    return Tcp_receiveMessage(buffer);
}

ssize_t
Tcp_sendMessage(char* message)
{
    return send(sockfd, message, strlen(message), 0);
}

ssize_t
Tcp_receiveMessage(char* buffer)
{
    return recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
}

ssize_t
Tcp_requestFile(char* fileName)
{
    Tcp_sendMessage(SEND_FILE);
    char fileSizeBuffer[MAX_BUFFER_SIZE];
    Tcp_receiveMessage(fileSizeBuffer);
    int fileSize = atoi(fileSizeBuffer);
    
    FILE* receivedFile = fopen(fileName, "w");

    if(receivedFile == NULL) {
        char errorMessage[512];
        snprintf(errorMessage, 512, "Could not open '%s' to write!", fileName);
        error(errorMessage);
        return -1;
    }

    int remainingData = fileSize;
    ssize_t len;
    char buffer[BUFSIZ];

    while((remainingData > 0) && ((len = recv(sockfd, buffer, BUFSIZ, 0)) > 0)) {
        fwrite(buffer, sizeof(char), len, receivedFile);
        remainingData -= len;

        #ifdef DEV
        printf("%d bytes left\n", remainingData);
        #endif
    }


    #ifdef DEV
    printf("done!\n");
    #endif

    fclose(receivedFile);
    return remainingData;
}