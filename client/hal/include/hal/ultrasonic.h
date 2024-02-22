/**
 * @file ultrasonic.h
 * @brief Control and read the HC-SR04 ultrasonic sensor.
 * @author Louie Lu 301291418
 */

// TODO:
// 1) Change Trigger to PWM
// 2) Implement hysteresis to avoid inaccurate, large swings in readings.

#include "hal/gpio.h"
#include "hal/timeutils.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#pragma once

/**
 * Configure the BeagleBone, then create a thread to start
 * the ultrasonic sensor.
 * @return bool Whether or not the init succeeded.
 */
bool
Ultrasonic_init(void);

/**
 * Shutdown the ultrasonic sensor thread.
 */
void
Ultrasonic_shutdown(void);

/**
 * Get the distance reading from the ultrasonic sensor in cm.
 */
long double
Ultrasonic_getDistanceInCm(void);