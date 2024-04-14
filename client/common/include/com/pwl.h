/**
 * @file pwl.h
 * @brief Piecewise Linear Functions.
 *
 * Provides a way to define and sample an arbitrary piecewise lienar function.
 * @author Spencer Leslie 301571329
 */
#pragma once

/** A piecwise linear function. */
typedef struct
{
    /** X coordinates. */
    float ptsX[6];
    /** Y coordinates. */
    float ptsY[6];
    /** Number of points specified. */
    int pts;
} Pwl_Function;

/** An ADSR envelope representing a string pluck. */
#define PWL_ADSR_PLUCK_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 0.1, 0.15, 0.65, 1.0 },                                 \
        .ptsY = { 0.0, 1.0, 0.6, 0.6, 0.0 }, .pts = 5                          \
    }

/** An ADSR envelope representing a singer singing an "Ahh" vowell. */
#define PWL_ADSR_AHH_FUNCTION                                                  \
    {                                                                          \
        .ptsX = { 0.0, 0.25, 0.35, 0.75, 1.0 },                                \
        .ptsY = { 0.0, 0.8, 0.55, 0.55, 0.0 }, .pts = 5                        \
    }

/** An ADSR envelope with a sharp attack and slow decay a la hitting a ringing
 * object with a hammer. */
#define PWL_ADSR_HAMMER_FUNCTION                                               \
    {                                                                          \
        .ptsX = { 0.0, 0.05, 0.1, 0.75, 1.0 },                                 \
        .ptsY = { 0.0, 1.0, 0.35, 0.2, 0.0 }, .pts = 5                         \
    }

/** A swell given as a linear rise and a linear fall. */
#define PWL_SWELL_FUNCTION                                                     \
    {                                                                          \
        .ptsX = { 0.0, 0.5, 1.0 }, .ptsY = { 0.0, 1.0, 0.0 }, .pts = 3         \
    }

/** An approximation of and exponential falloff given by e^-x (scaled and
 * translated). */
#define PWL_EXP_FALLOFF_FUNCTION                                               \
    {                                                                          \
        .ptsX = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0 }, .ptsY = { 1.0,               \
                                                            0.201896517994655, \
                                                            0.040762203978366, \
                                                            0.008229747049020, \
                                                            0.0,               \
                                                            0.0 },             \
        .pts = 6                                                               \
    }

/** A constant value.*/
#define PWL_CONST_FUNCTION                                                     \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 0.6, 0.6 }, .pts = 2                   \
    }

/** A sharp peak and a quick falloff. */
#define PWL_PEAKFALL_FUNCTION                                                  \
    {                                                                          \
        .ptsX = { 0.0, 0.1, 0.2, 0.3, 1.0 },                                   \
        .ptsY = { 0.0, 0.1, 0.18, 0.0, 0.0 }, .pts = 5                         \
    }

/** Linear falloff from y = 1 to y = 0 over [0, 1]. */
#define PWL_LINEARFALL_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 1.0, 0.0 }, .pts = 2                   \
    }

/** Linear falloff from y = 1 to y = -1 over [0, 1]. */
#define PWL_LINEARFALL_P1N1_FUNCTION                                           \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 1.0, -1.0 }, .pts = 2                  \
    }

/** Linear ramp from y = 0 to y = 1 over [0, 1] */
#define PWL_LINEARRAMP_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 0.0, 1.0 }, .pts = 2                   \
    }

/** Envelope representing the striking of a bell. */
#define PWL_BELLSTRIKE_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 0.25, 0.75, 1.0 }, .ptsY = { 0.8, 0.35, 0.35, 0.0 },    \
        .pts = 4                                                               \
    }

/** Sample a piecwise linear function at the given sample point. */
float
Pwl_sample(Pwl_Function* pwl, float samplePoint);
