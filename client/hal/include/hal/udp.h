#define PORT 12345
#define MAX_BUFFER_SIZE 1024
void
Udp_initializeUdpClient(char* hostname);

ssize_t
Udp_makeServerRequest(char* message, char* buffer);