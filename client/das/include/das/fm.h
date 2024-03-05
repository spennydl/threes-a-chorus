/**
 * @file fm.h
 * @brief Frequency-Modulation Synthesizer.
 *
 * This FM synthesizer consists of 4 operators, each of which independently
 * output a sine, saw, or square wave with an independent envelope at a
 * frequency relative to a base frequency. The operators can act as input to any
 * other operators or themselves, and any or all operators can be mixed together
 * and sound.
 *
 * @author Spencer Leslie
 */
#pragma once

#include "das/envelope.h"
#include "das/wavetable.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** A440 as a reference frequency. */
#define A_440 440.0

/** Notes. */
typedef enum
{
    A4 = 0,
    Bb4,
    B4,
    C4,
    Db4,
    D4,
    Eb4,
    E4,
    F4,
    Gb4,
    G4,
    Ab4,
    A5,
    NOTE_NONE
} Note;

/** Operators available. Adding more is as simple as adding more values here. */
typedef enum
{
    FM_OPERATOR0 = 0,
    FM_OPERATOR1,
    FM_OPERATOR2,
    FM_OPERATOR3,
    FM_OPERATORS
} FmOperator;

/** Parameters for each operator. */
typedef struct
{
    /**
     * The CM ratio, or the ratio between this operators frequency and the base
     * frequency.
     */
    double CmRatio;

    /**
     * How much this operator is mixed into the final output. Should be in
     * [0.0, 1.0].
     */
    double outputStrength;

    /**
     * Which operators should act as input to this operator. All values should
     * be in [0.0, 1.0].
     *
     * If O is a vector of the output values of all operators, the input value
     * to this operator will be the dot product of V and algorithmConnections.
     */
    double algorithmConnections[FM_OPERATORS];

    /** Type of the wave the operator should output. */
    WaveType waveType;

    /** The operator's envelope. */
    AdsrEnvelope adsr;
} OperatorParams;

/** Parameters for the synthesizer. */
typedef struct
{
    /** @brief The note the synthesizer should play. */
    Note note;

    /**
     * @brief The sample rate of the synthesizer.
     *
     * I don't see a reason not to sample at 44100hz, so this might move to a
     * constant at some point.
     */
    size_t sampleRate;

    /**
     * @brief Operator paramters.
     */
    OperatorParams opParams[FM_OPERATORS];
} FmSynthParams;

/**
 * @brief "Default" parameters.
 *
 * Configures OPERATOR0 to output a simple sine wave at A440.
 */
static const FmSynthParams FM_DEFAULT_PARAMS = {
    .sampleRate = 44100,
    .opParams = { { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 1.0,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 0.0 },
                    .adsr = { .attackMs = 400,
                              .decayMs = 100,
                              .releaseMs = 800,
                              .attackPeak = 0.75,
                              .sustainLevel = 0.55 } },
                  { .waveType = WAVETYPE_SAW,
                    .CmRatio = 2.0 / 1.0,
                    .algorithmConnections = { 0 },
                    .adsr = { .attackMs = 400,
                              .decayMs = 100,
                              .releaseMs = 800,
                              .attackPeak = 0.75,
                              .sustainLevel = 0.55 } },

                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 2.0,
                    .algorithmConnections = { 0 },
                    .adsr = { .attackMs = 400,
                              .decayMs = 100,
                              .releaseMs = 800,
                              .attackPeak = 0.75,
                              .sustainLevel = 0.55 } },
                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 2.0,
                    .algorithmConnections = { 0 },
                    .adsr = { .attackMs = 400,
                              .decayMs = 100,
                              .releaseMs = 800,
                              .attackPeak = 0.75,
                              .sustainLevel = 0.55 } } }
};

static const FmSynthParams FM_BASS_PARAMS = {
    .sampleRate = 44100,
    .opParams = { { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 4,
                    .outputStrength = 0.6,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 4 },
                    .adsr = { .attackMs = 200,
                              .decayMs = 100,
                              .releaseMs = 400,
                              .attackPeak = 0.75,
                              .sustainLevel = 0.55 } },
                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.0,
                    .algorithmConnections = { 0.0, 0.0, 2.4, 0.0 },
                    .adsr = { .attackMs = 200,
                              .decayMs = 100,
                              .releaseMs = 400,
                              .attackPeak = 0.75,
                              .sustainLevel = 0.55 } },

                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 4,
                    .outputStrength = 0,
                    .algorithmConnections = { 0 },
                    .adsr = { .attackMs = 200,
                              .decayMs = 100,
                              .releaseMs = 400,
                              .attackPeak = 0.75,
                              .sustainLevel = 0.55 } },
                  { .waveType = WAVETYPE_SAW,
                    .CmRatio = 1.0 / 4,
                    .outputStrength = 0,
                    .algorithmConnections = { 0 },
                    .adsr = { .attackMs = 200,
                              .decayMs = 100,
                              .releaseMs = 400,
                              .attackPeak = 0.5,
                              .sustainLevel = 0.55 } } }
};

/**
 * @brief External FmSythesizer handle.
 */
typedef struct
{
    void* __FmSynth;
} FmSynthesizer;

/**
 * @brief Create a new FM synth with default parameters.
 *
 * @return The FmSynthesizer or NULL on error.
 */
FmSynthesizer*
Fm_defaultSynthesizer(void);

/**
 * @brief Create a new FM synth with the given parameters.
 *
 * @return The FmSynthesizer or NULL on error.
 */
FmSynthesizer*
Fm_createFmSynthesizer(const FmSynthParams* params);

/**
 * @brief Destroy the synthesizer.
 *
 * @param synth The synth to destroy.
 */
void
Fm_destroySynthesizer(FmSynthesizer* synth);

void
Fm_setNote(FmSynthesizer* s, Note note);

/**
 * @brief Trigger the ADSR and start playing the currently configured note.
 *
 * This function is thread-safe.
 *
 * @param synth The synth.
 */
void
Fm_noteOn(FmSynthesizer* synth);

/**
 * @brief Gate the ADSR and stop playing.
 *
 * This function is thread-safe.
 *
 * @param synth The synth.
 */
void
Fm_noteOff(FmSynthesizer* synth);

/**
 * @brief Apply the given parameters to the synthesizer.
 *
 * This function is NOT thread-safe. Do not call it if @ref
 * Fm_generateSamples is running in another thread.
 *
 * @param synth The synth to update.
 * @param params The params to apply.
 */
void
Fm_updateParams(FmSynthesizer* synth, FmSynthParams* params);

/**
 * @brief Generate a given number of frames into a given buffer.
 *
 * @param synth The synth to use for generating.
 * @param sampleBuf The buffer to write samples to.
 * @param nSamples How many samples to write.
 */
void
Fm_generateSamples(FmSynthesizer* synth, int16_t* sampleBuf, size_t nSamples);
