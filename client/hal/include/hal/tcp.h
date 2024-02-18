#define PORT 12345
#define MAX_BUFFER_SIZE 1024
#define EXIT_CODE "TIMETOGOBYE"

void
Tcp_initializeTcpClient(char* hostname);

void
Tcp_cleanupTcpClient();

ssize_t
Tcp_makeServerRequest(char* message, char* buffer);