/**
 * @file singer.h
 * @brief Main singer application module.
 * @author Spencer Leslie and Louie Lu
 */
#pragma once

#include <stdatomic.h>
#include <stdbool.h>

/** Quandrants of the plane. Used to map a sensory index to a mood. */
typedef enum
{
    QUADRANT_NONE,
    QUADRANT_I,
    QUADRANT_II,
    QUADRANT_III,
    QUADRANT_IV,
} Quadrant;

/** The emotions a singer can be in. */
typedef enum
{
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_ANGRY,
    EMOTION_OVERSTIMULATED,
    EMOTION_NEUTRAL,
    EMOTION_SINGING,
} Emotion;

/** A singer's mood. */
typedef struct
{
    atomic_uint emotion; // Multiple modules want access to this.
    float magnitude;
} Mood;

/** Initialize the system. */
int
Singer_initialize(void);

/** Runs and returns the status code of the execution. */
int
Singer_update(void);

/** Starts the singer singing random melodies according to mood. */
void
Singer_sing(void);

/** Stops the singer's singing. */
void
Singer_rest(void);

/** Modulates the current synth voice according to the potentiometer and light
 * sensor reading. */
void
Singer_modulateVoice(void);

/** Set whether the segDisplay should be displaying the SINGING mood or not. */
void
SegDisplay_setIsSinging(bool isSinging);

/** Return a pointer to the current Mood struct of the singer. */
Mood*
Singer_getMood(void);

/** Shuts down the singer module. */
void
Singer_shutdown(void);
