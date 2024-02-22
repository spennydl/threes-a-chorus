#include "hal/ultrasonic.h"

#define PULSE_FREQUENCY_IN_MS 200
#define PULSE_WIDTH_IN_US 0.01
#define SPEED_OF_SOUND_IN_NS 343.0 / 1e9

// Local variables
static gpio triggerPin = { .linux_pin_number = 48,
                           .bbg_header = 9,
                           .bbg_pin_number = 15,
                           .direction = GPIO_OUT,
                           .__gpio = NULL }; // TODO: __gpio = NULL correct?
static gpio echoPin = { .linux_pin_number = 49,
                        .bbg_header = 9,
                        .bbg_pin_number = 23,
                        .direction = GPIO_IN,
                        .__gpio = NULL };

static bool initialized = false;
static pthread_t ultrasonic_t;

// Prototype
static void*
Ultrasonic_emitPulse(void* args);

/**
 * Emit a trigger pulse from the ultrasonic sensor
 * every 200ms, each with a duration of 0.01ms (10us).
 */
static void*
Ultrasonic_emitPulse(void* args)
{
    assert(initialized);
    (void)args;

    while (1) {
        Timeutils_sleepForMs(PULSE_FREQUENCY_IN_MS);
        gpio_write(&triggerPin, 1);
        Timeutils_sleepForMs(PULSE_WIDTH_IN_US);
        gpio_write(&triggerPin, 0);
    }
}

bool
Ultrasonic_init(void)
{
    gpio_open(&triggerPin);
    gpio_open(&echoPin);
    gpio_set_active_low(&echoPin, false);

    initialized = true;

    // Create thread
    int rc;
    if ((rc = pthread_create(
           &ultrasonic_t, NULL, Ultrasonic_emitPulse, NULL)) != 0) {
        perror("pthread_create:");
        initialized = false; // TODO: thoughts on this init fail handling?
    }

    return initialized;
}

void
Ultrasonic_shutdown(void)
{
    assert(initialized);

    pthread_cancel(ultrasonic_t);
    pthread_join(ultrasonic_t, NULL);
    initialized = false;
}

long double
Ultrasonic_getDistanceInCm(void)
{
    assert(initialized);

    long double startTimeInNs;
    long double endTimeInNs;
    long double elapsedTimeInNs;
    int echoValue;

    // echoValue automatically changes from 0 to 1
    // whenever the trigger pulse is emitted. It then
    // changes back to 0 when the pulse is received.
    do {
        gpio_read(&echoPin, &echoValue);
        startTimeInNs = Timeutils_getTimeInNs();
    } while (echoValue == 0);

    do {
        gpio_read(&echoPin, &echoValue);
        endTimeInNs = Timeutils_getTimeInNs();
    } while (echoValue == 1);

    elapsedTimeInNs = endTimeInNs - startTimeInNs;

    // Calculations (3):
    // distance = speed * time; halve the distance; convert m -> cm
    long double totalDistanceInM = SPEED_OF_SOUND_IN_NS * elapsedTimeInNs;
    long double distanceOfOneTripInM = totalDistanceInM / 2;
    long double distanceInCm = distanceOfOneTripInM * 100;

    // TODO: Could also do this at the expense of a magic number.
    // long double distanceInCm = elapsedTimeInNs * 0.000017150;

    // if (distanceInCm <= 0) {
    //     distanceInCm = 0;
    // }

    return distanceInCm;
}