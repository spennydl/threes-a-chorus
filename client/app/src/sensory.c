#include "sensory.h"
#include "com/pwl.h"
#include "com/timeutils.h"
#include "hal/accel.h"
#include "hal/adc.h"
#include "hal/button.h"
#include "hal/ultrasonic.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define NS_BETWEEN_SAMPLES (10000000)

// threshold between "gentle" and "rough" handling
#define CUBE_ROOT_0_25 0.62996052494
#define ACCEL_HIGH_THRESHOLD CUBE_ROOT_0_25

// NOTE: All rates and times are in ms/10.
// The main loop runs once every 10ms, and the rates are all specified as the
// number of times around the loop before updating.
// So, if you want to do something every 200ms, the update rate would be 20.

// how many interactions with the potentiometer we keep track of
// this esesentially sets the numerical resolution of the pot term
#define POTENTIOMETER_INTERACTION_THRESHOLD 10

// Which channel the light sensor is on.
#define LIGHT_SENSOR_CHANNEL ADC_CHANNEL3
// We quantize the light sensor readings to discrete levels. This sets
// how many levels to quantize to
#define LIGHT_SENSOR_LEVELS 10

// Update rates for each sensor and the interaction tolerance/sensory index.
#define LIGHT_SENSOR_UPDATE_RATE 20
#define DIST_SENSOR_UPDATE_RATE 20
#define POT_UPDATE_RATE 40
#define BUTTON_UPDATE_RATE 2
#define INTERACTION_TOLERANCE_UPDATE_RATE 10
// This shouldn't update too quickly or else the mood will swing around pretty
// wildly. 1s feels sorta right.
#define SENSORY_INDEX_UPDATE_RATE 100

/*****************************
 * Types
 *****************************/

/**
 * The state of interaction across all channels. Includes tolerance and sensory
 * index.
 */
struct InteractionState
{
    union
    {
        struct
        {
            Sensory_InputLevel accelState;
            Sensory_InputLevel lightLevel;
            Sensory_InputLevel proximityState;
            Sensory_InputLevel potInteractionState;
            Sensory_InputLevel buttonState;
        };
        Sensory_InputLevel states[5];
    };
    Sensory_SensoryIndex interactionTolerance;
    Sensory_SensoryIndex sensoryIndex;

    /** Current averaged and smoothed sensor input values */
    float sensorInputs[SENSORY_INPUTS];
    /** The constants to use for the weighted sum. */
    float constants[SENSORY_INPUTS];
    /** The piecewise linear function determining input tolerance over time. */
    Pwl_Function interactionToleranceFn;
};

// float constants[SENSORY_INPUTS] = { 10, -3, -3, 4, 5, -4 };

/**
 * @brief Integrates a discrete quantity over time and determines which of 3
 * levels the value is currently in.
 *
 * The integration serves as a debounce. For each state we must spend
 * a number of ticks equal to its countThreshold above its stateThreshold before
 * entering the state. Tuning the count threshold allows us to apply arbitrary
 * debounce times.
 */
struct TristateIntegrator
{
    float stateThresholds[3];
    int countThresholds[3];
    int counts[3];
    int triggers[3];
    int state;
};

/*****************************
 * Data
 *****************************/

/**
 * @brief A bounded counter.
 *
 * The value of the counter will not go above max or below zero. On update, the
 * counter will either be incremented or decremented; it never remains the same.
 * This is used as a way to count events in a rolling window. It's not quite the
 * best for this, but gets us close enough.
 */
struct IntegratingCounter
{
    int max;
    int value;
};

/** Handle to the worker thread. */
static pthread_t _senseThread;
/** Should we keep sensing? */
static int _sense = 0;

static int have_accel = 1;

/** Handle to the button. */
static Button* button;

/** The interaction state. */
static struct InteractionState _state = { 0 };

/** Counter for button events. We are polling every 20ms so cannot detect more
 * than 50/s. */
static struct IntegratingCounter _buttonEventCounter = { .max = 50,
                                                         .value = 0 };

/** Determines button interaction state. */
static struct TristateIntegrator _buttonStateIntegrator = {
    .stateThresholds = { 0.0, 0.01, 0.4 },
    .countThresholds = { 10, 3, 3 },
};

/** Determines light level state. */
static struct TristateIntegrator _lightLevelTsInt = {

    .stateThresholds = { 0.0, 0.1, 0.2 },
    .countThresholds = { 3, 3, 3 },
};

/** Determines accelerometer input state. */
static struct TristateIntegrator _accelTsInt = {
    .stateThresholds = { 0.0, 0.001, CUBE_ROOT_0_25 },
    .countThresholds = { 20, 20, 10 },
};

/** Determines distance sensor/proximity state. */
static struct TristateIntegrator _distTsInt = {
    .stateThresholds = { 0.0, 0.3, 0.65 },
    .countThresholds = { 3, 3, 3 },
};

/** Determines potentiometer interaction state. */
static struct TristateIntegrator _potInteractionIntegrator = {
    .stateThresholds = { 0.0, 0.01, 0.25 },
    .countThresholds = { 3, 3, 2 },
};

/*****************************
 * Prototypes
 *****************************/

/**
 * @brief Determines the global interaction level.
 *
 * Sums all interaction states and returns the sum.
 */
static int
_countInteractions(void);

/**
 * Adds the given data point to the tristate integrator and advances it.
 */
static void
_integrateTristate(struct TristateIntegrator* ti, float value);

/**
 * Updates an IntegratingCounter. The counter will be incremented if inc is
 * greater than zero.
 */
static void
_updateCounter(struct IntegratingCounter* counter, int inc);

/**
 * Samples the current light level and updates the light level state.
 */
static void
_sampleLightLevel(void);

/**
 * Samples the potentiometer and uses the difference between the current and
 * previous reading to update the potentiometer interaction state.
 */
static void
_updatePotReading(void);

/**
 * Takes a sample from the accelerometer and uses it to update the state.
 */
static void
_updateAccelerometer(void);

/**
 * Gets the current distance sensor reading and uses it to update the state.
 */
static void
_updateDistanceSensor(void);

/**
 * Checks for button events and uses them to update the button interaction
 * state.
 */
static void
_updateButton(void);

/**
 * Thread worker function that runs the main sensory integration loop.
 */
static void*
_senseWorker(void*);

/*****************************
 * Static fn impl's
 *****************************/

static int
_countInteractions(void)
{
    int count = 0;
    for (int i = 0; i < 5; i++) {
        // each state will be either 0, 1, or 2.
        // Adding them gives high interaction states
        // more weight.
        count += _state.states[i];
    }
    return count;
}

static void
_integrateTristate(struct TristateIntegrator* ti, float value)
{
    int stateIdx;
    int canTrigger = 1;
    // find which 'bucket' the given values lies in
    for (int i = 0; i < 3; i++) {
        float thresh = ti->stateThresholds[i];
        if (value >= thresh) {
            stateIdx = i;
        }
        if (ti->triggers[i] > 0) {
            canTrigger = 0;
        }
    }

    // update the counts
    for (int i = 0; i < 3; i++) {
        int count = ti->counts[i];
        count += (i == stateIdx) ? 1 : -1;
        if (count > ti->countThresholds[i]) {
            count = ti->countThresholds[i];
            ti->triggers[i] = (canTrigger) ? 1 : 0;
        }

        if (count < 0) {
            count = 0;
            ti->triggers[i] = 0;
        }

        ti->counts[i] = count;
    }

    // trigger a new state if we have one
    for (int i = 0; i < 3; i++) {
        if ((canTrigger) && ti->counts[i] == ti->countThresholds[i]) {
            ti->state = i;
        }
    }
}

static void
_updateCounter(struct IntegratingCounter* counter, int inc)
{
    int value = counter->value;
    value += (inc > 0) ? 1 : -1;

    if (value > counter->max) {
        value = counter->max;
    }

    if (value < 0) {
        value = 0;
    }

    counter->value = value;
}

static void
_sampleLightLevel(void)
{

    // get the sensor reading
    size_t level = adc_voltage_raw(LIGHT_SENSOR_CHANNEL);
    // quantize and normalize it
    size_t quantFactor = ADC_MAX_READING / LIGHT_SENSOR_LEVELS;
    size_t quantized = (level / quantFactor) * quantFactor;
    float normalized = (float)quantized / ADC_MAX_READING;

    // Offset so low levels are negative.
    // TODO: Ambient levels will make this a problem. Maybe we want to
    // only add values when they are <0.4 or >0.6? or something?
    // This should at least be easily calibrated, possibly in the cfg file.
    normalized -= 0.5;

    // Take a smoothed average
    float last = _state.sensorInputs[LIGHT_LEVEL];
    float val = (0.8 * normalized) + (0.2 * last);
    _state.sensorInputs[LIGHT_LEVEL] = val;

    // Further from zero should mean more interaction.
    // Integrate just the magnitude to get the interaction strength
    float mag = fabsf(val);
    _integrateTristate(&_lightLevelTsInt, mag);

    _state.lightLevel = _lightLevelTsInt.state;
}

static void
_updatePotReading(void)
{
    static float last;

    // Get the pot reading and find the difference from the last.
    // This tells us how far we've been swung which correlates to how fast
    // the pot is being turned.
    float pot = (float)adc_voltage_raw(ADC_CHANNEL0) / ADC_MAX_READING;
    float diff = fabsf(pot - last);
    diff = (diff <= 0.01) ? 0 : diff;

    last = pot;

    // Integrate.
    _integrateTristate(&_potInteractionIntegrator, diff);
    _state.potInteractionState = _potInteractionIntegrator.state;

    // Keep a smoothed average of the difference. We're smoothing pretty
    // aggresively just to keep the value from changing too fast.
    float avg = _state.sensorInputs[POTENTIOMETER_EVENTS];
    _state.sensorInputs[POTENTIOMETER_EVENTS] = (0.6 * diff) + (0.4 * avg);
}

static void
_updateAccelerometer(void)
{
    Accel_Sample accelSample;

    Accel_read(&accelSample);

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
    // Either way it would be better if this offset constant could be a
    // calibration parameter in the cfg file.
    float mag = sqrtf(x2 + y2 + z2) - 0.505;

    // take abs - we don't care about direction
    mag = fabsf(mag);

    // Take the cube root. Small changes will make big differences when being
    // handled gently, and small changes will make small differences when we're
    // being tossed around. not being tossed around.
    // TODO: keep this? It feels right, but we didn't end up non-linearizing the
    // others. We skipped doing that with the others since it didn't make a lot
    // of sense to for some and others (like the light sensor) are already
    // non-linear.
    mag = cbrt((mag < 0.005) ? 0 : mag);

    // integrate and update state.
    _integrateTristate(&_accelTsInt, mag);

    if (_accelTsInt.state == 2) { // TODO: constants
        _state.sensorInputs[ACCEL_HI] = mag;
        _state.sensorInputs[ACCEL_LO] = 0; // TODO does this make sense?
    } else {
        _state.sensorInputs[ACCEL_HI] = 0;
        _state.sensorInputs[ACCEL_LO] = mag;
    }
    _state.accelState = _accelTsInt.state;
}

static void
_updateDistanceSensor(void)
{
    double dist = Ultrasonic_getDistanceInCm();
    // TODO Bit of a kludge. getDist should probably just return zero eh?
    if (dist == INFINITY) {
        dist = 0;
    } else {
        // invert it in the range, since we want closer distances
        // to be "better" as far as the constant is concerned
        dist = 100.0 - dist;
        dist = dist / 100.0;
    }
    // Integrate and update.
    _integrateTristate(&_distTsInt, dist);
    _state.proximityState = _distTsInt.state;
    _state.sensorInputs[DISTANCE_SENSOR_READ] = dist;
}

static void
_updateButton(void)
{
    // event will be BUTTON_UP == 0 if no interaction.
    ButtonEvent event = Button_getState(button);
    _updateCounter(&_buttonEventCounter, event);

    // Normalize number of events.
    float eventsLastSec =
      (float)_buttonEventCounter.value / _buttonEventCounter.max;
    _state.sensorInputs[BUTTON_EVENTS] = eventsLastSec;

    // Update.
    _integrateTristate(&_buttonStateIntegrator, eventsLastSec);
    _state.buttonState = _buttonStateIntegrator.state;
}

static void*
_senseWorker(void* _unused)
{
    (void)_unused;

    float a = 0.7;
    float sensoryIndexHistory[SENSORY_INDEX_UPDATE_RATE];
    int count = 0;
    while (_sense) {
        long long start = Timeutils_getTimeInNs();

        if (have_accel) {
            _updateAccelerometer();
        }

        if (count % DIST_SENSOR_UPDATE_RATE == 0) {
            _updateDistanceSensor();
        }

        if (count % POT_UPDATE_RATE == 0) {
            _updatePotReading();
        }

        if (count % LIGHT_SENSOR_UPDATE_RATE == 0) {
            _sampleLightLevel();
        }

        if (count % BUTTON_UPDATE_RATE == 0) {
            _updateButton();
        }

        if (count % INTERACTION_TOLERANCE_UPDATE_RATE == 0) {
            float interactionLevel =
              (float)_countInteractions() / INTERACTION_TOLERANCE_UPDATE_RATE;
            _state.interactionTolerance =
              Pwl_sample(&_state.interactionToleranceFn, interactionLevel);
        }

        float newIdx = 0;
        for (int i = 0; i < SENSORY_INPUTS; i++) {
            newIdx += _state.sensorInputs[i] * _state.constants[i];
        }
        sensoryIndexHistory[count] = newIdx;
        count++;

        // integrate the history to get the current sensory index
        if (count % SENSORY_INDEX_UPDATE_RATE == 0) {
            float sum = 0;
            float oldIdx = _state.sensoryIndex;

            for (int i = 1; i < SENSORY_INDEX_UPDATE_RATE; i++) {
                sum +=
                  (sensoryIndexHistory[i] + sensoryIndexHistory[i - 1]) / 2;
            }
            _state.sensoryIndex = (a * sum) + ((1 - a) * oldIdx);

            count = 0;
        }

        long long elapsed = Timeutils_getTimeInNs() - start;
        long long toSleep = NS_BETWEEN_SAMPLES - elapsed;

        Timeutils_sleepForNs(toSleep);
    }
    return NULL;
}

/*****************************
 * Exported fn impl's
 *****************************/

int
Sensory_initialize(const Sensory_Preferences* prefs)
{
    if (Accel_open() < 0) {
        fprintf(stderr,
                "WARN: Could not open accelerometer. It will not work!\n");
        have_accel = 0;
    }

    if (!Ultrasonic_init()) {
        Accel_close();
        return SENSORY_EULTSON;
    }

    if ((button = Button_open(68, 8, 10)) == NULL) {
        Ultrasonic_shutdown();
        Accel_close();
        return -4;
    }

    Pwl_Function tolerance = PWL_LINEARFALL_P1N1_FUNCTION;
    _state.interactionTolerance = 0;
    _state.sensoryIndex = 0;
    memcpy(&_state.interactionToleranceFn, &tolerance, sizeof(Pwl_Function));

    _state.constants[ACCEL_LO] = prefs->cAccelLow;
    _state.constants[ACCEL_HI] = prefs->cAccelHigh;
    _state.constants[POTENTIOMETER_EVENTS] = prefs->cPot;
    _state.constants[DISTANCE_SENSOR_READ] = prefs->cDistance;
    _state.constants[LIGHT_LEVEL] = prefs->cLight;
    _state.constants[BUTTON_EVENTS] = prefs->cButton;

    return SENSORY_OK;
}

int
Sensory_beginSensing(void)
{
    _sense = 1;
    pthread_create(&_senseThread, NULL, _senseWorker, NULL);

    return 1;
}

void
Sensory_reportSensoryState(Sensory_State* state)
{
    state->accelState = _state.accelState;
    state->lightLevel = _state.lightLevel;
    state->proximityState = _state.proximityState;
    state->potInteractionState = _state.potInteractionState;
    state->buttonState = _state.buttonState;
    state->sensoryIndex = _state.sensoryIndex;
    state->sensoryTolerance = _state.interactionTolerance;
}

const char*
Sensory_inputLevelToStr(Sensory_InputLevel lvl)
{
    switch (lvl) {
        case NEUTRAL: {
            return "NEUTRAL";
            break;
        }
        case LOW: {
            return "LOW";
            break;
        }
        case HI: {
            return "HI";
            break;
        }
        default: {
            return "UNKNOWN LEVEL";
            break;
        }
    }
}

void
Sensory_close(void)
{
    _sense = 0;
    pthread_join(_senseThread, NULL);

    Button_close(button);
    Ultrasonic_shutdown();
    Accel_close();
}
