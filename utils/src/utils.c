#include "utils.h"

#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095

void Utils_runCommand(char *command)
{
    // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
        // printf("--> %s", buffer); // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
    }
}

long long Utils_getTimeInMs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;

    return milliSeconds;
}

long double Utils_getTimeInNs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;

    long long totalNanoSeconds = seconds * 1000000000 + nanoSeconds;
    return totalNanoSeconds;
}

void Utils_sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;

    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;

    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}

int Utils_getRandomIntBtwn(int min, int max)
{
    // Reference:
    // https://stackoverflow.com/questions/822323/how-to-generate-a-random-int-in-c
    // Used only as a starting point.

    // Example shown with max = 3000, min = 500
    int range = max - min;                   // 2500
    int rangeInclusive = range + 1;          // 2501
    int randomInt = rand() % rangeInclusive; // 0    -> 2500
    int randomIntInRange = randomInt + min;  // 500  -> 3000

    return randomIntInRange;
}

double Utils_convertA2dToVoltage(int a2dReading)
{
    double a2dPercentage = (double)a2dReading / A2D_MAX_READING;
    double voltage = a2dPercentage * A2D_VOLTAGE_REF_V;
    return voltage;
}