#include "hal/ultrasonic.h"
#include "hal/gpio.h"
#include "hal/timeutils.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#define TEN_US_IN_NS 10000
#define DUTY_CYCLE_10US 0.00005
#define NS_TO_CM_CONVERSION 0.000017150
#define ONE_METER 100
// Should never be longer than 200ms since we're pulsing
// the sensor at 200ms.
#define GPIO_LONG_TIMEOUT 200

// Local variables
static gpio triggerPin = { .linux_pin_number = 48,
                           .bbg_header = 9,
                           .bbg_pin_number = 15,
                           .direction = GPIO_OUT,
                           .__gpio = NULL };
static gpio echoPin = { .linux_pin_number = 49,
                        .bbg_header = 9,
                        .bbg_pin_number = 23,
                        .direction = GPIO_IN,
                        .__gpio = NULL };

static bool initialized = false;

static _Atomic double lastReadDistance;

static pthread_t sensorThread;

static int
waitForTriggerExpecting(int expected)
{
    int echoValue;

    int timeout = GPIO_LONG_TIMEOUT;
    int waitStatus = gpio_wait(&echoPin, 1, &echoValue, timeout);
    if (waitStatus < 0) {
        fprintf(stderr,
                "WARN: Ultrasonic thread got GPIO wait error %d\n",
                waitStatus);
        return 0;
    } else if (waitStatus == 0) { // we've timed out.
        // This isn't an error, it just means that there's nothing reflecting
        // the sound back.
        return 0;
    }

    if (echoValue != expected) { // expected. Start of pulse.
        fprintf(stderr,
                "WARN: unexpected pulse value - expected %d but got %d\n",
                expected,
                echoValue);
        return -1;
    }
    return 1;
}
static void*
distanceLoop(void* unused)
{
    (void)unused;

    while (initialized) {
        Timeutils_sleepForMs(500);
        // let's not spam this quite so much.
        gpio_write(&triggerPin, 1);
        Timeutils_sleepForNs(TEN_US_IN_NS);
        gpio_write(&triggerPin, 0);

        double startTimeInNs;
        double endTimeInNs;
        double elapsedTimeInNs;

        int echoValue;
        if ((echoValue = waitForTriggerExpecting(1)) <= 0) {
            lastReadDistance = INFINITY;
            continue;
        }
        startTimeInNs = Timeutils_getTimeInNs();

        if ((echoValue = waitForTriggerExpecting(0)) <= 0) {
            lastReadDistance = INFINITY;
            continue;
        }
        endTimeInNs = Timeutils_getTimeInNs();

        // TODO: Tried gpio_wait(). The readings didn't work well for me but I
        // might be missing something. gpio gpioPins[1] = { echoPin }; int
        // outVals[1];

        // startTimeInNs = Timeutils_getTimeInNs();
        // int eventsReceived = gpio_wait(gpioPins, 1, outVals, 1000);
        // endTimeInNs = Timeutils_getTimeInNs();

        elapsedTimeInNs = endTimeInNs - startTimeInNs;

        double distance = elapsedTimeInNs * NS_TO_CM_CONVERSION;
        if (distance > ONE_METER) {
            distance = INFINITY;
        }

        lastReadDistance = distance;
    }
    return NULL;
}

bool
Ultrasonic_init(void)
{
    gpio_open(&echoPin);
    gpio_set_active_low(&echoPin, false);

    if (gpio_open(&triggerPin) < 0) {
        printf("can't init trig\n");
    }

    if (gpio_write(&triggerPin, 0) < 0) {
        printf("can't write trig\n");
    }

    lastReadDistance = INFINITY;

    initialized = true;

    pthread_create(&sensorThread, NULL, distanceLoop, NULL);

    return initialized;
}

void
Ultrasonic_shutdown(void)
{
    assert(initialized);
    initialized = false;
    pthread_join(sensorThread, NULL);

    gpio_close(&echoPin);
    gpio_close(&triggerPin);

    initialized = false;
}

long double
Ultrasonic_getDistanceInCm(void)
{
    return lastReadDistance;
}
