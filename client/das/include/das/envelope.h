/**
 * @file envelope.h
 * @brief ADSR Envelope Generator.
 *
 * Calculates and updates a linear ADSR envelope.
 *
 * I'm not particularly happy with this implementation, it deserves an overhaul.
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
