/**
 * @file ultrasonic.h
 * @brief Control and read the HC-SR04 ultrasonic sensor.
 * @author Louie Lu 301291418
 */

// TODO:
// 2) Implement hysteresis to avoid inaccurate, large swings in readings.
#pragma once

#include <pthread.h>
#include <stdbool.h>
/**
 * Configure the BeagleBone's PWM and GPIO for the ultrasonic sensor.
 * PWM is used by the trigger to emit a pulse for 10us every 200ms.
 * GPIO is used by the echo to receive the trigger pulse.
 * @return bool Whether or not the init succeeded.
 */
bool
Ultrasonic_init(void);

/**
 * Shutdown the ultrasonic sensor.
 */
void
Ultrasonic_shutdown(void);

/**
 * Get the distance reading from the ultrasonic sensor in cm.
 * @return long double Distance to the nearest object in cm.
 */
long double
Ultrasonic_getDistanceInCm(void);
