#pragma once

#include "das/wavetable.h"
#include <stddef.h>
#include <stdint.h>

#define A_440 440.0

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
    A5
} Note;

typedef enum
{
    FM_OPERATOR0 = 0,
    FM_OPERATOR1,
    FM_OPERATOR2,
    FM_OPERATOR3,
    FM_OPERATORS
} FmOperator;

typedef struct
{
    double strength;
    WaveType waveType;
    unsigned char C;
    unsigned char M;
} OperatorParams;

typedef struct
{
    Note note;
    size_t sampleRate;
    WaveType carrierWaveType;
    OperatorParams opParams[FM_OPERATORS];
} FmSynthParams;

static const FmSynthParams FM_DEFAULT_PARAMS = {
    .note = A4,
    .sampleRate = 44100,
    .carrierWaveType = WAVETYPE_SINE,
    .opParams = { { .strength = 0.0,
                    .waveType = WAVETYPE_SINE,
                    .C = 1,
                    .M = 2 },
                  { .strength = 0.0,
                    .waveType = WAVETYPE_SINE,
                    .C = 1,
                    .M = 2 },
                  { .strength = 0.0,
                    .waveType = WAVETYPE_SINE,
                    .C = 1,
                    .M = 2 },
                  { .strength = 0.0,
                    .waveType = WAVETYPE_SINE,
                    .C = 1,
                    .M = 2 } }
};

/// External-facing type for the fm synth
typedef struct
{
    void* __FmSynth;
} FmSynthesizer;

/// Create a new FM synth with default parameters.
FmSynthesizer*
Fm_defaultSynthesizer(void);

/// Create a new FM synth
FmSynthesizer*
Fm_createFmSynthesizer(const FmSynthParams* params);

/// Destry an FM synth
void
Fm_destroySynthesizer(FmSynthesizer* synth);

/// Connect an operator to another
void
Fm_connectOperators(FmSynthesizer* synth, FmOperator from, FmOperator to);

/// Set the carrier note
void
Fm_setNote(FmSynthesizer* synth, Note note);

/// Sets the frequency of an operator relative to the frequency of the carrier.
/// The desired frequency is computed as a ratio between the operator and the
/// carrier where C is the carrier (numerator) and M is the operator
/// (denominator)
void
Fm_setOperatorCM(FmSynthesizer* synth,
                 FmOperator
                 operator,
                 unsigned char C,
                 unsigned char M);

void
Fm_triggerNote(FmSynthesizer* synth);
void
Fm_gateNote(FmSynthesizer* synth);

/// Get the frequency of an operator.
double
Fm_getOpFrequency(FmSynthesizer* synth, FmOperator op);

/// Explicitly set the frequency of an oeprator.
void
Fm_setOpFrequency(FmSynthesizer* synth, FmOperator op, double freq);

void
Fm_setOpStrength(FmSynthesizer* synth, FmOperator op, double strength);

/// Generate n_samples samples in the sample_buf
void
Fm_generateSamples(FmSynthesizer* synth, int16_t* sampleBuf, size_t nSamples);
