/*
 * Contains utility functions, including:
 *      -running a Linux command
 *      -time
 *      -random
 *      -conversions
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef _UTILS_H_
#define _UTILS_H_

void Utils_runCommand(char *command);

long long Utils_getTimeInMs(void);
long double Utils_getTimeInNs(void);

void Utils_sleepForMs(long long delayInMs);

int Utils_getRandomIntBtwn(int min, int max);

double Utils_convertA2dToVoltage(int a2dReading);

#endif