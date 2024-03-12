/**
 * @file envelope.h
 * @brief ADSR Envelope Generator.
 *
 * Calculates and updates a linear ADSR envelope.
 *
 * I'm not particularly happy with this implementation, it deserves an overhaul.
 *
 * TODO: rewrite this!
 *
 * I think the idea of attack, decay, sustain, and release all as distinct
 * phases is too specific to be useful.
 *
 * In an ideal world, we would allow the user to specify an arbitrary curve and
 * a gate point. When the envelope is triggered, we would simply sample the
 * curve until we reach the gate point, at which time we would simply play back
 * the sample at the gate point. When a gate event is received we would then
 * play the rest of the curve. Further, this approach also supports looping the
 * envelope, effectively turning them into arbitrary low-frequency oscillators.
 *
 * Specifying and sampling an arbitrary curve could be done using besier curves,
 * but that feels pretty out of scope for this project and difficult to
 * configure given that we don't have a UI. Instead, I would like to
 * re-implement this using wave tables as a comprimise.
 *
 * Moving forward, the plan would be:
 * - Generate some functions we can use as envelopes and create wave tables out
 * of them. Useful functions to have may be linear ADSR (eg, the classic thing
 * we've implemented here), exponential falloff (e^-x), a "swell" (parbaps an
 * upside-down parabola, or a linear ramp up/down), etc.
 * - Allow the user to specify an envelope as 3 (optionally 4) pieces of data:
 *   - A function.
 *   - The length of the envelope (so we stretch them horizontally in time)
 *   - The gate point - e.g. how far through the envelope we get before we
 *     require gate to continue
 *   - A loop point, if we wish to loop the envelope. If present, instead of
 *   - waiting for a gate signal when we reach the gate point we would simply
 *     return to the loop point and keep playing.
 *
 * This feels like a good compromise. We would lose the ability to specify
 * arbitrary envelopes, but we can provide baked envelopes that are common and
 * useful, and the implementation should be a snap.
 *
 * @author Spencer Leslie
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#define ENV_ADSR_PLUCK_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 0.1, 0.15, 0.65, 1.0 },                                 \
        .ptsY = { 0.0, 0.75, 0.55, 0.55, 0.0 }, .pts = 5                       \
    }

#define ENV_ADSR_AHH_FUNCTION                                                  \
    {                                                                          \
        .ptsX = { 0.0, 0.25, 0.35, 0.75, 1.0 },                                \
        .ptsY = { 0.0, 0.6, 0.55, 0.55, 0.0 }, .pts = 5                        \
    }

#define ENV_SWELL_FUNCTION                                                     \
    {                                                                          \
        .ptsX = { 0.0, 0.5, 1.0 }, .ptsY = { 0.0, 1.0, 0.0 }, .pts = 3         \
    }

#define ENV_EXP_FALLOFF_FUNCTION                                               \
    {                                                                          \
        .ptsX = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0 }, .ptsY = { 1.0,               \
                                                            0.201896517994655, \
                                                            0.040762203978366, \
                                                            0.008229747049020, \
                                                            0.001661557273174, \
                                                            0.0 },             \
        .pts = 6                                                               \
    }

#define ENV_CONST_FUNCTION                                                     \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 0.6, 0.6 }, .pts = 2                   \
    }

#define ENV_PEAKFALL_FUNCTION                                                  \
    {                                                                          \
        .ptsX = { 0.0, 0.1, 0.2, 0.3, 1.0 },                                   \
        .ptsY = { 0.0, 0.1, 0.18, 0.0, 0.0 }, .pts = 5                         \
    }

#define ENV_LINEARFALL_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 1.0, 0.0 }, .pts = 2                   \
    }

#define ENV_LINEARRAMP_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 0.0, 1.0 }, .pts = 2                   \
    }

#define ENV_BELLSTRIKE_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 0.25, 0.75, 1.0 }, .ptsY = { 0.6, 0.35, 0.35, 0.0 },    \
        .pts = 4                                                               \
    }

typedef struct
{
    float ptsX[6];
    float ptsY[6];
    int pts;
} Env_PiecewiseLinearFn;

typedef struct
{
    float step;
    float current;
    float gatePoint;
    float repeatPoint;
    long lengthMs;

    Env_PiecewiseLinearFn fn;

    uint8_t state;
} Env_Envelope;

void
Env_initEnvelope(Env_Envelope* env, size_t sampleRate);

float
Env_getValueAndAdvance(Env_Envelope* env);

void
Env_trigger(Env_Envelope* env);

void
Env_gate(Env_Envelope* env);
