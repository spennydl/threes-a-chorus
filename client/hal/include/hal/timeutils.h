/**
 * @file timeutils.h
 * @brief Contains util functions related to time.
 */

#include <time.h>

#pragma once

long long
Timeutils_getTimeInMs(void);

long double
Timeutils_getTimeInNs(void);

void
Timeutils_sleepForMs(long long delayInMs);

void
Timeutils_sleepForNs(long long delayInNs);