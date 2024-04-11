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

#include "com/pwl.h"
#include "das/envelope.h"
#include "das/wavetable.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** A440 as a reference frequency. */
#define A_440 440.0

#define C2_HZ 65.41

/** Notes. */
typedef enum
{
    C2 = 0,
    Db2,
    D2,
    Eb2,
    E2,
    F2,
    Gb2,
    G2,
    Ab2,
    A2,
    Bb2,
    B2,

    C3,
    Db3,
    D3,
    Eb3,
    E3,
    F3,
    Gb3,
    G3,
    Ab3,
    A3,
    Bb3,
    B3,

    C4,
    Db4,
    D4,
    Eb4,
    E4,
    F4,
    Gb4,
    G4,
    Ab4,
    A4,
    Bb4,
    B4,

    C5,
    Db5,
    D5,
    Eb5,
    E5,
    F5,
    Gb5,
    G5,
    Ab5,
    A5,
    Bb5,
    B5,

    C6,
    Db6,
    D6,
    Eb6,
    E6,
    F6,
    Gb6,
    G6,
    Ab6,
    A6,
    Bb6,
    B6,
} NoteOffset;

#define NOTE_NONE INT_MIN

typedef int Note;

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
    float CmRatio;

    /**
     * How much this operator is mixed into the final output. Should be in
     * [0.0, 1.0].
     */
    float outputStrength;

    /**
     * Which operators should act as input to this operator. All values should
     * be in [0.0, 1.0].
     *
     * If O is a vector of the output values of all operators, the input value
     * to this operator will be the dot product of V and algorithmConnections.
     */
    float algorithmConnections[FM_OPERATORS];

    /** Type of the wave the operator should output. */
    WaveType waveType;

    Note fixToNote;
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

    Env_Envelope opEnvelopes[FM_OPERATORS];
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
                    .algorithmConnections = { 0.0, 440.0, 0.0, 0.0 } },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .algorithmConnections = { 0 },
                  },
                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 2.0,
                    .algorithmConnections = { 0 } },
                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 2.0,
                    .algorithmConnections = { 0 } } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 1.0,
                       .repeatPoint = 0,
                       .lengthMs = 1000,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION }

    }
};

static const FmSynthParams FM_PIANO_PARAMS = {
    .sampleRate = 44100,
    .opParams = { { .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.5,
                    .algorithmConnections = { 0, 853, 220, 220 } },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.0,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 200.0 },
                  },
                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 2.5,
                    .outputStrength = 0.3,
                    .algorithmConnections = { 0.0, 880.0, 0.0, 0.0 } },
                  { .waveType = WAVETYPE_SINE,
                    .CmRatio = 4,
                    .outputStrength = 0.0,
                    .algorithmConnections = { 0, 0, 0, 0 } } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_ADSR_HAMMER_FUNCTION },
                     { .gatePoint = 0.8,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_CONST_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_BELLSTRIKE_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_ADSR_AHH_FUNCTION }

    }
};

static const FmSynthParams FM_SAWBLADE_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SAW,
                    .CmRatio = 2.50,
                    .outputStrength = 0.2,
                    .algorithmConnections = { 0.0, 69, 0.0, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SAW,
                    .CmRatio = 0.25,
                    .outputStrength = 0.1,
                    .algorithmConnections = { 0, 0, 420, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 0.5,
                    .outputStrength = 0.3,
                    .algorithmConnections = { 0, 0, 0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SAW,
                    .CmRatio = 3.5,
                    .outputStrength = 0.3,
                    .algorithmConnections = { 0, 0, 1760, 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_ADSR_HAMMER_FUNCTION },
                     { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_ADSR_AHH_FUNCTION },
                     { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 1300,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 1200,
                       .fn = PWL_ADSR_HAMMER_FUNCTION } }
};

static const FmSynthParams FM_BELL_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.5,
                    .algorithmConnections = { 0.0, 800, 0.0, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 3.5,
                    .algorithmConnections = { 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.5,
                    .algorithmConnections = { 0, 0, 0, 300 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 3.5,
                    .algorithmConnections = { 0, 0, 0, 100 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.75,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_LINEARFALL_FUNCTION },
                     { .gatePoint = 0.75,
                       .repeatPoint = -1,
                       .lengthMs = 1200,
                       .fn = PWL_BELLSTRIKE_FUNCTION },
                     { .gatePoint = 0.75,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_BELLSTRIKE_FUNCTION },
                     { .gatePoint = 0.75,
                       .repeatPoint = -1,
                       .lengthMs = 1300,
                       .fn = PWL_BELLSTRIKE_FUNCTION } }
};

static const FmSynthParams FM_CRY_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1,
                    .fixToNote = E2,
                    .outputStrength = 1.0,
                    .algorithmConnections = { 0.0, 300, 0.0, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .algorithmConnections = { 0, 0.0, 200, 100 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 9.0,
                    .algorithmConnections = { 0, 0, 0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 3.0 / 2.0,
                    .algorithmConnections = { 0, 0, 0, 160 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.85,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_LINEARFALL_FUNCTION },
                     { .gatePoint = 0.75,
                       .repeatPoint = -1,
                       .lengthMs = 1200,
                       .fn = PWL_BELLSTRIKE_FUNCTION },
                     { .gatePoint = 0.75,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_BELLSTRIKE_FUNCTION },
                     { .gatePoint = 1.0,
                       .repeatPoint = 0,
                       .lengthMs = 200,
                       .fn = PWL_SWELL_FUNCTION } }
};

/**
 * @brief "Ahhhh" parameters.
 */
static const FmSynthParams FM_AHH_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.5,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 120 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 11.0,
                    .outputStrength = 0.3,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 760 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 18.0,
                    .outputStrength = 0.2,
                    .algorithmConnections = { 0, 0, 0, 1130 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .algorithmConnections = { 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_AHH_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_AHH_FUNCTION },
                     { .gatePoint = 1,
                       .repeatPoint = 0,
                       .lengthMs = 1600,
                       .fn = PWL_CONST_FUNCTION } }
};

static const FmSynthParams FM_BASS_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 2,
                    .outputStrength = 0.6,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 440 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 2.0,
                    .outputStrength = 0.3,
                    .algorithmConnections = { 0.0, 0.0, 230, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 2,
                    .algorithmConnections = { 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 2.0,
                    .algorithmConnections = { 0, 0, 140, 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_EXP_FALLOFF_FUNCTION } }
};

static const FmSynthParams FM_BRASS_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.6,
                    .fixToNote = NOTE_NONE,
                    .algorithmConnections = { 0.0,
                                              (8 * 233.1),
                                              (8 * 233.1),
                                              0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.0,
                    .fixToNote = NOTE_NONE,
                    .algorithmConnections = { 0.0, 0.0, (6 * 233.1), 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 3.0,
                    .outputStrength = 0,
                    .fixToNote = NOTE_NONE,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 4.0,
                    .outputStrength = 0.2,
                    .fixToNote = NOTE_NONE,
                    .algorithmConnections = { 0, (8 * 233.1), 0, 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_AHH_FUNCTION },
                     { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_AHH_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_AHH_FUNCTION } }
};

static const FmSynthParams FM_YOI_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.51,
                    .algorithmConnections = { 0.0, 4097.0, 0.0, 490.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.68,
                    .algorithmConnections = { 0.0, 0.0, 1287, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SQUARE,
                    .CmRatio = 1,
                    .algorithmConnections = { 0 },
                  },
                  {
                    .waveType = WAVETYPE_SAW,
                    .CmRatio = 8.0 / 5,
                    .algorithmConnections = { 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.1,
                       .repeatPoint = 0.0,
                       .lengthMs = 2000,
                       .fn = PWL_PEAKFALL_FUNCTION },
                     { .gatePoint = 0.5,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_PEAKFALL_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_EXP_FALLOFF_FUNCTION } }
};

static const FmSynthParams FM_BIG_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SQUARE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.8,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 380 },
                  },
                  {
                    .waveType = WAVETYPE_SQUARE,
                    .CmRatio = 1.0 / 20.0,
                    .outputStrength = 0.3,
                    .algorithmConnections = { 0.0, 0.0, 450, 30 },
                  },
                  {
                    .waveType = WAVETYPE_SQUARE,
                    .CmRatio = 1,
                    .algorithmConnections = { 0, 0, 140, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 2.0,
                    .algorithmConnections = { 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 1500,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 0.8,
                       .repeatPoint = 0.2,
                       .lengthMs = 3000,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 1.0,
                       .repeatPoint = 0,
                       .lengthMs = 7000,
                       .fn = PWL_SWELL_FUNCTION }

    }
};

static const FmSynthParams FM_GLITCHBOOP_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SQUARE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.7,
                    .algorithmConnections = { 0.0, 1250, 0, 304 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 100.0,
                    .algorithmConnections = { 0.0, 0.0, 0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SAW,
                    .CmRatio = 1.0 / 9,
                    .outputStrength = 0.15,
                    .algorithmConnections = { 0, 0, 0, 24 },
                  },
                  {
                    .waveType = WAVETYPE_SAW,
                    .CmRatio = 1.0 / 27,
                    .algorithmConnections = { 0, 0, 0, 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 1500,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 1.0,
                       .repeatPoint = 0,
                       .lengthMs = 2345,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.8,
                       .repeatPoint = 0.2,
                       .lengthMs = 1000,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = 0.1,
                       .lengthMs = 3200,
                       .fn = PWL_SWELL_FUNCTION } }

};

static const FmSynthParams FM_BEEPBOOP_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.7,
                    .algorithmConnections = { 0.0, 250, 0, 304 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 100.0,
                    .algorithmConnections = { 0.0, 0.0, 0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0 / 9,
                    .outputStrength = 0.15,
                    .algorithmConnections = { 0, 0, 0, 24 },
                  },
                  {
                    .waveType = WAVETYPE_SQUARE,
                    .CmRatio = 1.0 / 53,
                    .algorithmConnections = { 0, 0, 0, 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.65,
                       .repeatPoint = -1,
                       .lengthMs = 1500,
                       .fn = PWL_ADSR_PLUCK_FUNCTION },
                     { .gatePoint = 1.0,
                       .repeatPoint = 0,
                       .lengthMs = 2345,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.8,
                       .repeatPoint = 0.2,
                       .lengthMs = 1000,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = 0.1,
                       .lengthMs = 3200,
                       .fn = PWL_SWELL_FUNCTION } }

};

static const FmSynthParams FM_SHINYDRONE_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 3.0,
                    .outputStrength = 0.5,
                    .algorithmConnections = { 0.0, 2000, 0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 4.0,
                    .algorithmConnections = { 0.0, 0.0, 0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 6.0,
                    .outputStrength = 0.5,
                    .algorithmConnections = { 0, 0, 0, 2000 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 2.5,
                    .algorithmConnections = { 0, 0, 0, 2000 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.8,
                       .repeatPoint = 0.2,
                       .lengthMs = 1000,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.8,
                       .repeatPoint = 0.2,
                       .lengthMs = 800,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = 0.1,
                       .lengthMs = 2500,
                       .fn = PWL_SWELL_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = 0.1,
                       .lengthMs = 3700,
                       .fn = PWL_SWELL_FUNCTION } }

};

static const FmSynthParams FM_CHIRP_PARAMS = {
    .sampleRate = 44100,
    .opParams = { {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.2,
                    .algorithmConnections = { 0.0, 1760, 0, 0.0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 1.0,
                    .outputStrength = 0.4,
                    .algorithmConnections = { 0.0, 0.0, 0.0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 3,
                    .outputStrength = 0.15,
                    .algorithmConnections = { 380, 0, 0, 0 },
                  },
                  {
                    .waveType = WAVETYPE_SINE,
                    .CmRatio = 6.0,
                    .algorithmConnections = { 0, 0, 0, 0 },
                  } },
    .opEnvelopes = { { .gatePoint = 0.2,
                       .repeatPoint = -1,
                       .lengthMs = 1000,
                       .fn = PWL_PEAKFALL_FUNCTION },
                     { .gatePoint = 0.3,
                       .repeatPoint = -1,
                       .lengthMs = 830,
                       .fn = PWL_EXP_FALLOFF_FUNCTION },
                     { .gatePoint = 0.2,
                       .repeatPoint = -1,
                       .lengthMs = 800,
                       .fn = PWL_PEAKFALL_FUNCTION },
                     { .gatePoint = 0.9,
                       .repeatPoint = -1,
                       .lengthMs = 600,
                       .fn = PWL_CONST_FUNCTION } }

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

void
Fm_updateOpParams(FmSynthesizer* s,
                  FmOperator op,
                  const OperatorParams* params);

/**
 * @brief Generate a given number of frames into a given buffer.
 *
 * @param synth The synth to use for generating.
 * @param sampleBuf The buffer to write samples to.
 * @param nSamples How many samples to write.
 */
void
Fm_generateSamples(FmSynthesizer* synth, int16_t* sampleBuf, size_t nSamples);
