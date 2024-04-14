/**
 * @file timeutils.h
 * @brief Contains util functions related to time.
 * @author Louie Lu
 */

#pragma once

/** Get the current system time in milliseconds. */
long long
Timeutils_getTimeInMs(void);

/** Get the current system time in nanoseconds. */
long double
Timeutils_getTimeInNs(void);

/** Sleep the current thread for the given number of milliseconds. */
void
Timeutils_sleepForMs(long long delayInMs);

/** Sleep the current thread for the given number of nanoseconds. */
void
Timeutils_sleepForNs(long long delayInNs);
