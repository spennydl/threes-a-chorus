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

/**
 * @brief Current state of the envelope, i.e. which "phase" it's in.
 */
typedef enum
{
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    ADSR_OFF
} _AdsrState;

/**
 * @brief ADSR data.
 */
typedef struct
{
    size_t attackMs;
    size_t decayMs;
    size_t releaseMs;
    double sustainLevel;
    double attackPeak;

    double step;
    double envelope;
    size_t updatesPerSec;

    _AdsrState state;
} AdsrEnvelope;

/**
 * @brief Prepares the ADSR for use.
 *
 * The ADSR struct passed to this function should have the attackMs, decayMs,
 * releaseMs, sustainLevel, and attackPeak parameters set on it. This function
 * prepares the internal data required to run the ADSR envelope.
 *
 * @param adsr The ADSR to initialize.
 * @param updatesPerSecond How many times per second this envelope will be
 * undated.
 * @param sampleRate The sample rate of the synthesizer..
 */
void
Env_adsrInit(AdsrEnvelope* adsr, size_t updatesPerSecond, size_t sampleRate);

/**
 * @brief Update the ADSR and get the current envelope value.
 *
 * Advances the ADSR envelope and returns the loudness.
 *
 * @param adsr The envelope to update.
 * @return double The current envelope value in [0.0, 1.0].
 */
double
Env_adsrUpdate(AdsrEnvelope* adsr);

/**
 * @brief Trigger the envelope.
 *
 * Puts the ADSR in ATTACK state and sets the envelope to 0. This corresponds to
 * the state at the beginning of a note.
 * @param adsr The adsr.
 */
void
Env_adsrTrigger(AdsrEnvelope* adsr);

/**
 * @brief Gate the ADSR.
 *
 * Puts the ADSR in the RELEASE phase and updates the step to the falloff rate.
 *
 * @param adsr The ADSR to gate.
 */
void
Env_adsrGate(AdsrEnvelope* adsr);
