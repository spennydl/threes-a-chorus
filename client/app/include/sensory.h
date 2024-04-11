/**
 * @file sensory.h
 * @brief Sensory integration module.
 *
 * Polls sensors and integrates the readings into a "sensory index" that
 * represensents how "pleased" the system is with the sensory input it is
 * receiving.
 *
 * All sensor readings are pretty aggressively smoothed. We do this because we
 * need to poll them quickly to catch short events, however we want the moods to
 * update much more slowly so that it's a little more coherent.
 *
 * The module exposes the current sensory index and the interaction level of
 * each of the sensor channels.
 *
 * @author Spencer Leslie 301571329
 */
#pragma once

#include "com/pwl.h"

/** The type of the sensory index value. */
typedef float Sensory_SensoryIndex;

/** Error codes for the system. */
#define SENSORY_OK 0
#define SENSORY_EACCEL -1
#define SENSORY_EULTSON -2

/** Sensory input channels. */
typedef enum
{
    ACCEL_LO,
    ACCEL_HI,
    POTENTIOMETER_EVENTS,
    DISTANCE_SENSOR_READ,
    LIGHT_LEVEL,
    BUTTON_EVENTS,
    SENSORY_INPUTS
} Sensory_Inputs;

/** Possible input levels for a channel. */
typedef enum
{
    NEUTRAL = 0,
    LOW,
    HI
} Sensory_InputLevel;

/** The current state of the system. */
typedef struct
{
    /** The input levels of each sensor as NEUTRAL, LOW, or HIGH. */
    Sensory_InputLevel accelState;
    Sensory_InputLevel lightLevel;
    Sensory_InputLevel proximityState;
    Sensory_InputLevel potInteractionState;
    Sensory_InputLevel buttonState;

    /** The sensory index. This represents how "pleased" the system is with the
     * total input. */
    Sensory_SensoryIndex sensoryIndex;
    /**
     * Sensory tolerance over time. This represents how the system feels about
     * the level of interaction over time. It could enjoy interaction more or
     * less as time goes.
     */
    Sensory_SensoryIndex sensoryTolerance;
} Sensory_State;

/**
 * Specifies the sensory preferences of the system.
 */
typedef struct
{
    float cAccelLow;
    float cAccelHigh;
    float cPot;
    float cDistance;
    float cLight;
    float cButton;

    Pwl_Function toleranceFunction;
    // TODO: the PWL tolerance function should go here.
} Sensory_Preferences;

/**
 * Initializes the sensory system and configures it for the given preferences.
 */
int
Sensory_initialize(const Sensory_Preferences* prefs);

/**
 * Spins up the sensing thread and start sensing.
 */
int
Sensory_beginSensing(void);

/**
 * Copies the current interaction state into the given Sensory_State.
 */
void
Sensory_reportSensoryState(Sensory_State* state);

/**
 * Gets a string representation of the Sensory_InputLevel.
 */
const char*
Sensory_inputLevelToStr(Sensory_InputLevel lvl);

/**
 * Stops sensing and cleans up.
 */
void
Sensory_close(void);

float
Sensory_getPotLevel(void);

float
Sensory_getLightReading(void);
