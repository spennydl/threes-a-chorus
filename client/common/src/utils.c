/*
 * @file utils.c
 * @brief Implementation of utility functions.
 */
#include "com/utils.h"
#include <stdlib.h>

int
Utils_getRandomIntBtwn(int min, int max)
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
