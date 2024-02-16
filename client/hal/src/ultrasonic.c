#include "hal/ultrasonic.h"

// TODO: These filepaths should be generalized
#define TRIG_DIRECTION_FILEPATH "/sys/class/gpio/gpio48/direction"
#define ECHO_DIRECTION_FILEPATH "/sys/class/gpio/gpio49/direction"

#define TRIG_VALUE_FILEPATH "/sys/class/gpio/gpio48/value"
#define ECHO_VALUE_FILEPATH "/sys/class/gpio/gpio49/value"

#define ECHO_ACTIVE_LOW_FILEPATH "/sys/class/gpio/gpio49/active_low"

#define PULSE_FREQUENCY_IN_MS 200
#define PULSE_WIDTH_IN_US 0.01
#define SPEED_OF_SOUND_IN_NS 343.0 / 1e9

static bool initialized = false;
static pthread_t ultrasonic_t;

void Ultrasonic_init(void)
{
    // Configure pins for GPIO
    Utils_runCommand("config-pin p9.15 gpio");
    Utils_runCommand("config-pin p9.23 gpio");

    // Set pin directions
    File_write(TRIG_DIRECTION_FILEPATH, "out");
    File_write(ECHO_DIRECTION_FILEPATH, "in");

    // Flip the active-low meaning (0 = OFF, 1 = ON)
    File_write(ECHO_ACTIVE_LOW_FILEPATH, "0");

    initialized = true;

    pthread_create(&ultrasonic_t, NULL, Ultrasonic_emitPulse, NULL);
}

void Ultrasonic_shutdown(void)
{
    assert(initialized);

    pthread_cancel(ultrasonic_t);
    pthread_join(ultrasonic_t, NULL);

    initialized = false;
}

void *Ultrasonic_emitPulse()
{
    assert(initialized);

    // As suggested by the guide, emit pulse every 200ms
    // for a duration of 0.01ms (10us). From my own testing,
    // < 200ms results in less accurate readings when the distance
    // changes drastically. In-class methods of reducing noise
    // would help (though I haven't impl. those in A2 yet).
    while (1) {
        Utils_sleepForMs(PULSE_FREQUENCY_IN_MS);
        File_write(TRIG_VALUE_FILEPATH, "1");
        Utils_sleepForMs(PULSE_WIDTH_IN_US);
        File_write(TRIG_VALUE_FILEPATH, "0");
    }
}

long double Ultrasonic_getDistanceInCm(void)
{
    assert(initialized);

    long double startTimeInNs;
    long double endTimeInNs;
    long double elapsedTimeInNs;

    while (File_readInt(ECHO_VALUE_FILEPATH) == 0) {
        startTimeInNs = Utils_getTimeInNs();
    }

    while (File_readInt(ECHO_VALUE_FILEPATH) == 1) {
        endTimeInNs = Utils_getTimeInNs();
    }

    elapsedTimeInNs = endTimeInNs - startTimeInNs;

    // distance = speed * time; halve the distance; convert m -> cm
    long double totalDistanceInM = SPEED_OF_SOUND_IN_NS * elapsedTimeInNs;
    long double distanceOfOneTripInM = totalDistanceInM / 2;
    long double distanceInCm = distanceOfOneTripInM * 100;

    // TODO: Could also do this at the expense of a magic number.
    // long double distanceInCm = elapsedTimeInNs * 0.000017150;

    if (distanceInCm <= 0) {
        distanceInCm = 0;
    }

    return distanceInCm;
}