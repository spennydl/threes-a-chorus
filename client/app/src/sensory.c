#include "sensory.h"
#include "hal/accel.h"
#include "hal/adc.h"
#include "hal/timeutils.h"
#include "hal/ultrasonic.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define NS_BETWEEN_SAMPLES (10000000)

// threshold between "gentle" and "rough" handling
#define ACCEL_HIGH_THRESHOLD 0.2
#define CUBE_ROOT_0_25 0.62996052494
// 100ms debounce on the way up?
#define ACCEL_LOHI_UP_THRESHOLD 10
// long debounce (1.5s) on the way down
#define ACCEL_LOHI_DOWN_THRESHOLD 150
// how many interactions with the potentiometer we keep track of
// this esesentially sets the numerical resolution of the pot term
#define POTENTIOMETER_INTERACTION_THRESHOLD 10

typedef enum
{
    ACCEL_LO,
    ACCEL_HI,
    POTENTIOMETER_EVENTS,
    DISTANCE_SENSOR_READ,
    SENSORY_INPUTS
} SensoryInputs;

static pthread_t _senseThread;
static int _sense = 0;

static float _sensoryIndex;

static float _sensorInputs[SENSORY_INPUTS] = { 0 };
static float _constants[SENSORY_INPUTS] = { 10, -3, -3, 4 };
static char report[128];

typedef struct
{
    int thresholdUp;
    int thresholdDown;
    int value;
    int trigger;
} Integrator;
static Integrator _potEventIntegrator = { .thresholdUp =
                                            POTENTIOMETER_INTERACTION_THRESHOLD,
                                          .thresholdDown =
                                            POTENTIOMETER_INTERACTION_THRESHOLD,
                                          .value = 0,
                                          .trigger = 0 };
static Integrator _accelLoHiIntegrator = { .thresholdUp =
                                             ACCEL_LOHI_UP_THRESHOLD,
                                           .thresholdDown =
                                             ACCEL_LOHI_DOWN_THRESHOLD,
                                           .value = 0,
                                           .trigger = 0 };

typedef enum
{
    INTEGRATOR_TRIGGER,
    INTEGRATOR_RELEASE,
} IntegratorEvent;

static void
_integrate(Integrator* integrator, int val)
{
    int intVal = integrator->value;
    int threshold =
      integrator->trigger ? integrator->thresholdDown : integrator->thresholdUp;
    if (val != 0) {
        intVal += 1;
        if (intVal > threshold) {
            integrator->value = integrator->thresholdDown;
            integrator->trigger = 1;
        } else {
            integrator->value = intVal;
        }
    } else {
        intVal -= 1;
        if (intVal < 0) {
            integrator->value = 0;
            integrator->trigger = 0;
        } else {
            integrator->value = intVal;
        }
    }
}

static void*
_senseWorker(void*);

static void
_printReport(void);
static void
_printReport(void)
{
    snprintf(report,
             128,
             "hi: %9.6f  lo: %9.6f  pot: %9.6f ult: %9.6f idx: %9.2f",
             _sensorInputs[ACCEL_HI],
             _sensorInputs[ACCEL_LO],
             _sensorInputs[POTENTIOMETER_EVENTS],
             _sensorInputs[DISTANCE_SENSOR_READ],
             _sensoryIndex);
    printf("%s", report);
}

static void
_clearReport(void)
{
    int len = strnlen(report, 128);
    for (int i = 0; i < len; i++) {
        putc('\b', stdout);
    }
}

static void
_updatePotReading(void)
{
    static float last;

    float pot = (float)adc_voltage_raw(ADC_CHANNEL0) / ADC_MAX_READING;

    float diff = pot - last;
    int val = (diff > 0.01 || diff < -0.01);
    last = pot;

    _integrate(&_potEventIntegrator, val);

    float normalized =
      (float)_potEventIntegrator.value / POTENTIOMETER_INTERACTION_THRESHOLD;
    _sensorInputs[POTENTIOMETER_EVENTS] = normalized;
}

static void
_updateAccelerometer(void)
{
    Accel_Sample accelSample;

    if (Accel_read(&accelSample) < 0) {
        // handle the error please.
    }

    // Normalize the 3 axes to [-1, 1]
    float xnorm = (float)accelSample.x / (-1 * ACCEL_MIN_READING);
    float x2 = xnorm * xnorm;

    float ynorm = (float)accelSample.y / (-1 * ACCEL_MIN_READING);
    float y2 = ynorm * ynorm;

    float znorm = (float)accelSample.z / (-1 * ACCEL_MIN_READING);
    float z2 = znorm * znorm;

    // Take vector mag and compensate for gravity. It will always feel 1G
    // due to gravity.
    // TODO: The accelerometer reads ever so slightly high. Maybe we need
    // to compensate each differently? or enable a filter?
    float mag = sqrtf(x2 + y2 + z2) - 0.505;

    // take abs - we don't care about direction
    float sign = copysignf(1.0, mag);
    mag *= sign;

    mag = cbrt((mag < 0.005) ? 0 : mag);

    // set low and high threshold
    //
    // TODO: We need to debounce this too.
    //   Right now HI doesn't have as much say as LO since we go thru LO once on
    //   the way to HI and once on the way down from HI.
    //   We need to debounce for a good while after we hit HI.
    int vv = (mag < CUBE_ROOT_0_25) ? 0 : 1;
    _integrate(&_accelLoHiIntegrator, vv);

    // if triggered, we are being roughly handled
    if (_accelLoHiIntegrator.trigger) {
        _sensorInputs[ACCEL_HI] = mag;
        _sensorInputs[ACCEL_LO] = 0; // TODO does this make sense?
    } else {
        _sensorInputs[ACCEL_HI] = 0;
        _sensorInputs[ACCEL_LO] = mag;
    }
}

static void*
_senseWorker(void* _unused)
{
    (void)_unused;
    float a = 0.7;
    float history[100];
    int count = 0;
    float oldIdx = 0;
    int updatePot = 0;
    int updatePotEvery = 40;
    while (_sense) {
        long long start = Timeutils_getTimeInNs();
        double dist = Ultrasonic_getDistanceInCm();
        // TODO should probably just be zero then eh?
        if (dist == INFINITY) {
            dist = 0;
        } else {
            // invert it in the range, since we want closer distances
            // to be "better" as far as the constant is concerned
            dist = 100.0 - dist;
            dist = dist / 100.0;
        }
        _sensorInputs[DISTANCE_SENSOR_READ] = dist;

        _updateAccelerometer();
        if (updatePot == updatePotEvery) {
            _updatePotReading();
            updatePot = 0;
        }
        updatePot++;

        _printReport();

        // TODO: we're integrating sensory index, but we really
        // want to keep history and integrate the sensor readings themselves
        float newIdx = 0;
        for (int i = 0; i < SENSORY_INPUTS; i++) {
            newIdx += _sensorInputs[i] * _constants[i];
        }
        history[count] = (a * newIdx) + ((1 - a) * oldIdx);
        oldIdx = newIdx;
        count++;

        // integrate the history to get the current sensory index
        if (count == 100) {
            float sum = 0;
            for (int i = 1; i < 100; i++) {
                sum += (history[i] + history[i - 1]) / 2;
            }
            count = 0;
            _sensoryIndex = sum;
        }

        long long elapsed = Timeutils_getTimeInNs() - start;
        long long toSleep = NS_BETWEEN_SAMPLES - elapsed;
        Timeutils_sleepForNs(toSleep);

        _clearReport();
    }
    return NULL;
}

int
Sensory_initialize(void)
{
    if (Accel_open() < 0) {
        return SENSORY_EACCEL;
    }

    if (!Ultrasonic_init()) {
        Accel_close();
        return SENSORY_EULTSON;
    }

    return SENSORY_OK;
}

int
Sensory_beginSensing(void)
{
    _sense = 1;
    pthread_create(&_senseThread, NULL, _senseWorker, NULL);

    return 1;
}

Sensory_SensoryIndex
Sensory_getSensoryIndex(void)
{
    // TODO sync
    return _sensoryIndex;
}

void
Sensory_close(void)
{
    _sense = 0;
    pthread_join(_senseThread, NULL);
}
