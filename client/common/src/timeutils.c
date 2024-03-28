#include "com/timeutils.h"
#include <time.h>

#define NS_PER_SECOND 1000000000
#define NS_PER_MS (1000 * 1000)

long long
Timeutils_getTimeInMs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;

    return milliSeconds;
}

long double
Timeutils_getTimeInNs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;

    long long totalNanoSeconds = seconds * 1000000000 + nanoSeconds;
    return totalNanoSeconds;
}

void
Timeutils_sleepForMs(long long delayInMs)
{
    long long delayNs = delayInMs * NS_PER_MS;
    Timeutils_sleepForNs(delayNs);
}

void
Timeutils_sleepForNs(long long delayNs)
{
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;

    struct timespec reqDelay = { seconds, nanoseconds };
    nanosleep(&reqDelay, (struct timespec*)NULL);
}
