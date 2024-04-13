/**
 * @file ultrasonicExample.h
 * @brief An example function that reads the ultrasonic sensor distance.
 */

#include "hal/ultrasonic.h"

#include <stdio.h>

void
ultrasonicExample(void)
{
    Ultrasonic_init();

    while (1) {
        long double distanceInCm = Ultrasonic_getDistanceInCm();
        if (distanceInCm != 0) {
            printf("Distance: %.1Lf cm\n", distanceInCm);
        }
    }

    Ultrasonic_shutdown();
}