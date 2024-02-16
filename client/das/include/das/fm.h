#ifndef TAC_DAS_FM_H
#define TAC_DAS_FM_H

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
} note;

typedef enum
{
    FM_OPERATOR0 = 0,
    FM_OPERATOR1,
    FM_OPERATOR2,
    FM_OPERATOR3,
    FM_OPERATORS
} fm_operator;

typedef struct
{
    unsigned char first;
    unsigned char second;
} pair_uc;

typedef struct
{
    double strength;
    wave_type wave_type;
    pair_uc CM;
} operator_params;

typedef struct
{
    note note;
    size_t sample_rate;
    wave_type carrier_wave_type;
    operator_params op_params[FM_OPERATORS];
} fm_params;

static const fm_params FM_DEFAULT_PARAMS = {
    .note = A4,
    .sample_rate = 44100,
    .carrier_wave_type = WAVETYPE_SINE,
    .op_params = { { .strength = 0.0,
                     .wave_type = WAVETYPE_SINE,
                     .CM = { .first = 1, .second = 2 } },
                   { .strength = 0.0,
                     .wave_type = WAVETYPE_SINE,
                     .CM = { .first = 1, .second = 2 } },
                   { .strength = 0.0,
                     .wave_type = WAVETYPE_SINE,
                     .CM = { .first = 1, .second = 2 } },
                   { .strength = 0.0,
                     .wave_type = WAVETYPE_SINE,
                     .CM = { .first = 1, .second = 2 } } }

};

/// External-facing type for the fm synth
typedef void fm;

/// Create a new FM synth with default parameters.
fm*
fm_default(void);

/// Create a new FM synth
fm*
fm_new(const fm_params* params);

/// Destry an FM synth
void
fm_destroy(fm** synth);

/// Connect an operator to another
void
fm_connect(fm* synth, fm_operator from, fm_operator to);

/// Set the carrier note
void
fm_set_note(fm* synth, note note);

/// Sets the frequency of an operator relative to the frequency of the carrier.
/// The desired frequency is computed as a ratio between the operator and the
/// carrier where C is the carrier (numerator) and M is the operator
/// (denominator)
void
fm_set_operator_CM(fm* synth, fm_operator operator, pair_uc CM_ratio);

/// Get the frequency of an operator.
double
fm_get_operator_freq(fm* synth, fm_operator op);

/// Explicitly set the frequency of an oeprator.
void
fm_set_operator_freq(fm* synth, fm_operator op, double freq);

void
fm_set_operator_strength(fm* synth, fm_operator op, double strength);

/// Generate n_samples samples in the sample_buf
void
fm_generate_samples(fm* synth, int16_t* sample_buf, size_t n_samples);

#endif // TAC_DAS_FM_H
