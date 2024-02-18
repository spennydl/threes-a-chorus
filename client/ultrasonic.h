/*
 * Provides an interface to control and read the HC-SR04 ultrasonic sensor.
 */

#include "fileUtils.h"
#include "utils.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h> // TODO: Remove later, just for testing

#ifndef _ULTRASONIC_H_
#define _ULTRASONIC_H_

/**
 * Configure the BeagleBone, then create a thread to start
 * the ultrasonic sensor.
 */
void Ultrasonic_init(void);

/**
 * Shutdown the ultrasonic sensor thread.
 */
void Ultrasonic_shutdown(void);

/**
 * Emit a trigger pulse continuously from the ultrasonic sensor.
 * (TODO: This function lacks params such as frequency and pulseWidth
 * because I'm not sure how to pass in params to threaded functions.
 * The values are hard-coded for now, 200ms frequency and 10us pulseWidth,
 * as suggested by the guide.)
 */
void *Ultrasonic_emitPulse();

/**
 * Get the distance reading from the ultrasonic sensor in cm.
 * (TODO: I believe the observer pattern would be nice to only
 * return non-0 distance values here, but I haven't impl. that in A2 yet.)
 */
long double Ultrasonic_getDistanceInCm(void);

#endif