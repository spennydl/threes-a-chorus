#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <hal/tcp.h>

static int sockfd;

static unsigned int serverlen;
static struct sockaddr_in serverAddress;

static struct hostent* server;

static pthread_mutex_t tcpLock = PTHREAD_MUTEX_INITIALIZER;

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
Tcp_initializeTcpClient(const char* hostname)
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

    // If we need to set a timeout in the future
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

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
    pthread_mutex_lock(&tcpLock);
    Tcp_sendMessage(message);
    bzero(buffer, MAX_BUFFER_SIZE);
    ssize_t res = Tcp_receiveMessage(buffer);
    pthread_mutex_unlock(&tcpLock);
    return res;
}

ssize_t
Tcp_sendMessage(char* message)
{
    char msg[MAX_BUFFER_SIZE] = { 0 };
    strncpy(msg, message, strlen(message));
    return send(sockfd, msg, MAX_BUFFER_SIZE, 0);
}

ssize_t
Tcp_receiveMessage(char* buffer)
{
    return recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
}

ssize_t
Tcp_requestFile(char* fileName)
{
    pthread_mutex_lock(&tcpLock);
    Tcp_sendMessage(SEND_FILE);
    char fileSizeBuffer[MAX_BUFFER_SIZE];

    Tcp_receiveMessage(fileSizeBuffer);
    int fileSize = atoi(fileSizeBuffer);

    FILE* receivedFile = fopen(fileName, "w");

    if (receivedFile == NULL) {
        char errorMessage[512];
        snprintf(errorMessage, 512, "Could not open '%s' to write!", fileName);
        error(errorMessage);
        pthread_mutex_unlock(&tcpLock);
        return -1;
    }

    int remainingData = fileSize;

    char buffer[BUFSIZ];

    ssize_t len;

    while (remainingData > 0) {
        len = recv(
          sockfd, buffer, remainingData < BUFSIZ ? remainingData : BUFSIZ, 0);

        if (len <= 0) {
            perror("Ran into error while recv file");
            fclose(receivedFile);
            pthread_mutex_unlock(&tcpLock);
            return len;
        }

        fwrite(buffer, sizeof(char), len, receivedFile);

        remainingData -= len;
#ifdef DEV
        printf("%d bytes got so %d bytes left\n", len, remainingData);
#endif
    }

#ifdef DEV
    printf("done!\n");
#endif

    fclose(receivedFile);
    pthread_mutex_unlock(&tcpLock);

    return remainingData;
}
