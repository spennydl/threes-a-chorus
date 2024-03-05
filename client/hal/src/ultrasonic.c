#include "hal/ultrasonic.h"

#define DUTY_CYCLE_10US 0.00005
#define NS_TO_CM_CONVERSION 0.000017150

// Local variables
static pwm triggerChannel = { .channel = PWM_CHANNEL0B, .__pwm = NULL };
static gpio echoPin = { .linux_pin_number = 49,
                        .bbg_header = 9,
                        .bbg_pin_number = 23,
                        .direction = GPIO_IN,
                        .__gpio = NULL };

static bool initialized = false;

bool
Ultrasonic_init(void)
{
    gpio_open(&echoPin);
    gpio_set_active_low(&echoPin, false);

    pwm_open(&triggerChannel);
    pwm_set_duty_cycle(&triggerChannel, DUTY_CYCLE_10US);
    pwm_set_period(&triggerChannel, 5);
    pwm_enable(&triggerChannel, true);

    initialized = true;

    return initialized;
}

void
Ultrasonic_shutdown(void)
{
    assert(initialized);

    gpio_close(&echoPin);
    pwm_close(&triggerChannel);

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

    // TODO: Tried gpio_wait(). The readings didn't work well for me but I might
    // be missing something.
    // gpio gpioPins[1] = { echoPin };
    // int outVals[1];

    // startTimeInNs = Timeutils_getTimeInNs();
    // int eventsReceived = gpio_wait(gpioPins, 1, outVals, 1000);
    // endTimeInNs = Timeutils_getTimeInNs();

    elapsedTimeInNs = endTimeInNs - startTimeInNs;

    long double distanceInCm = elapsedTimeInNs * NS_TO_CM_CONVERSION;

    return distanceInCm;
}