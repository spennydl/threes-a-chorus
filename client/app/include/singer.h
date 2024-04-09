/**
 * @file singer.h
 * @brief Main singer application module.
 * @author Spencer Leslie 301571329
 */
#pragma once

#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum
{
    QUADRANT_NONE,
    QUADRANT_I,
    QUADRANT_II,
    QUADRANT_III,
    QUADRANT_IV,
} Quadrant;

typedef enum
{
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_ANGRY,
    EMOTION_OVERSTIMULATED,
    EMOTION_NEUTRAL,
    EMOTION_SINGING,
} Emotion;

typedef struct
{
    atomic_uint emotion; // I'm anticipating that both segDisplay and
                         // melodygen might want to access this, so it's atomic.
    float magnitude;
} Mood;

/** Initialize the system. */
int
Singer_initialize(void);

/** Runs and returns the status code of the execution. */
int
Singer_update(void);

/** Set whether the segDisplay should be displaying the SINGING mood or not. */
void
SegDisplay_setIsSinging(bool isSinging);

/** Return a pointer to the current Mood struct of the singer. */
Mood*
Singer_getMood(void);

void
Singer_shutdown();
