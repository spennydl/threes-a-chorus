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

#include "com/pwl.h"

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    float step;
    float current;
    float gatePoint;
    float repeatPoint;
    long lengthMs;

    Pwl_Function fn;

    uint8_t state;
} Env_Envelope;

void
Env_prepareEnvelope(Env_Envelope* env, size_t sampleRate);

float
Env_getValueAndAdvance(Env_Envelope* env);

void
Env_trigger(Env_Envelope* env);

void
Env_gate(Env_Envelope* env);
