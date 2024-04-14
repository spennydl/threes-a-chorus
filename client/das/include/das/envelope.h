/**
 * @file envelope.h
 * @brief ADSR Envelope Generator.
 *
 * Calculates a sound loudness envelope. Envelopes are given as a piecewise
 * linear function, a gate point, a total length in milliseconds, and an
 * optional repeat point.
 *
 * When triggered, the envelope will advance until it reaches the gate point. If
 * a repeat point is specified, the envelope will return to the repeat point and
 * play back again. Otherwise it will be held at the gate point until the
 * envelope is gated.
 *
 * Once an envelope is gated, whether or not a repeat point is specified, it
 * will be allowed past the gate point and will run to completion.
 *
 * @author Spencer Leslie 3015713429
 */
#pragma once

#include "com/pwl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** The envelope. */
typedef struct
{
    // "Private" data. Do not touch these.
    /** The step that the sample point is moved by each time the envelope
     * advances. Should not be changed externally. */
    float step;
    /** The current sample point. */
    float current;
    /** Used to specify a "minimum" value to combat "clicking" that occurs when
     * an envelope is retriggered and suddenly drops to 0. */
    float min;

    // "public" data. Set these before calling Env_prepareEnvelope.
    /** The envelope's gate point. */
    float gatePoint;
    /** The envelope's repeat point. The envelope will repeat if the repeat
     * point is greater than 0.*/
    float repeatPoint;
    /** Length of the envelope im milliseconds. */
    int lengthMs;
    /** The PWL specifying the envelope. */
    Pwl_Function fn;

    // "private" data. Included at the end as to not mess with alignment.
    /** Current envelope state. */
    uint8_t state;
} Env_Envelope;

/**
 * Prepares the envelope for use.
 *
 * @param env The envelope to set up. Before calling this function,
 *            env->gatePoint, env->repeatPoint, env->lengthMs, and env->fn
 *            should all have been set.
 * @param sampleRate The rate in samples/second that the envelope will be
 * sampled at.
 */
void
Env_prepareEnvelope(Env_Envelope* env, size_t sampleRate);

/**
 * Samples the current value of the envelope and advances it.
 *
 * @param env The envelope to sample and advance.
 * @return float The envelope value.
 */
float
Env_getValueAndAdvance(Env_Envelope* env);

/**
 * Trigger and envelope. If the envelope was already running, it will be
 * retriggered and start again at 0.
 *
 * @param env The envelope to trigger.
 */
void
Env_trigger(Env_Envelope* env);

/**
 * Gate the envelope, allowing it to run to completion.
 *
 * @param env The envelope to gate.
 */
void
Env_gate(Env_Envelope* env);
