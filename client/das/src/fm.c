#include "das/fm.h"

#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TWELVETH_ROOT_OF_TWO 1.059463094359

typedef struct
{
    note note;
    wave_type carrier_wave;
    size_t sample_rate;
    size_t attack_ms;
    size_t decay_ms;
    size_t release_ms;

    // TODO: These don't have to be quite so big, ya?
    // QUESTION: Do we want ADSR to be a wavetable?
    //
    // 5 * 8 = 40, and with the above 24 we're at 64.
    double sustain_level;
    double attack_level;
    double carrier_freq;
    double carrier_angle;
    double carrier_step;

    double op_enveleope[FM_OPERATORS];
    double op_angle[FM_OPERATORS];
    double op_step[FM_OPERATORS];
    double op_CM[FM_OPERATORS];
    double op_strength[FM_OPERATORS];

    wave_type op_wave[FM_OPERATORS];
    double mod_by[FM_OPERATORS * FM_OPERATORS];

    size_t op_attack_ms[FM_OPERATORS];
    size_t op_decay_ms[FM_OPERATORS];
    size_t op_release_ms[FM_OPERATORS];
    double op_attack_level[FM_OPERATORS];
    double op_sustain_level[FM_OPERATORS];
} _fm;

inline static double
_calc_step(double freq, size_t sample_rate);
static _fm*
_fm_configure(_fm* synth, const fm_params* params);
static _fm*
_fm_alloc(void);

static _fm*
_fm_alloc(void)
{
    _fm* synth = malloc(sizeof(_fm));
    if (synth) {
        memset(synth, 0, sizeof(_fm));
    }
    return synth;
}

inline static double
_calc_step(double freq, size_t sample_rate)
{
    // samples/sec / periods/sec(freq) = samples/period.
    return PI2 / (sample_rate / freq);
}

static _fm*
_fm_configure(_fm* synth, const fm_params* params)
{
    if (NULL != synth) {
        fm_set_note(synth, params->note);
        synth->sample_rate = params->sample_rate;
        synth->carrier_step =
          _calc_step(synth->carrier_freq, synth->sample_rate);
        synth->sustain_level = 0.75; // TODO: Envelopes.
        synth->carrier_wave = params->carrier_wave_type;

        for (int op = 0; op < FM_OPERATORS; op++) {
            const operator_params* op_params = &params->op_params[op];
            synth->op_wave[op] = op_params->wave_type;
            synth->op_strength[op] = op_params->strength;

            fm_set_operator_CM(synth, op, op_params->CM);
            synth->op_sustain_level[op] = 0.75; // TODO: Envelopes.
        }
    }
    return synth;
}

fm*
fm_default(void)
{
    _fm* synth = _fm_alloc();
    return _fm_configure(synth, &FM_DEFAULT_PARAMS);
}

fm*
fm_new(const fm_params* params)
{
    _fm* synth = _fm_alloc();
    return _fm_configure(synth, params);
}

void
fm_destroy(fm** synth)
{
    if (NULL != *synth) {
        free(*synth);
        *synth = NULL;
    }
}

/// Connect an operator to another
void
fm_connect(fm* s, fm_operator from, fm_operator to)
{
    // TODO implement
    (void)s;
    (void)from;
    (void)to;
}

/// Set the carrier note
void
fm_set_note(fm* s, note note)
{
    _fm* synth = s;
    // Frequency of a note relative to a reference frequency is given by:
    //
    // $$ F * 2^{N / 12} $$
    //
    // where:
    // - F is the reference note frequency
    // - N is how many half-steps away (positive or negative) the target note is
    //   from the reference note.
    synth->carrier_freq = A_440 * powf(TWELVETH_ROOT_OF_TWO, note);
    synth->carrier_step = _calc_step(synth->carrier_freq, synth->sample_rate);
    synth->note = note;

    for (int op = 0; op < FM_OPERATORS; op++) {
        fm_set_operator_freq(synth, op, synth->carrier_freq * synth->op_CM[op]);
    }
}

// TODO: Do we need to continue to support both specific operator frequencies as
// well as ratios?
//
// Either way this will need some cleaning up.
void
fm_set_operator_CM(fm* s, fm_operator operator, pair_uc CM)
{
    _fm* synth = s;
    double ratio = (double)CM.second / CM.first;
    synth->op_CM[operator] = ratio;
    double op_freq = synth->carrier_freq * ratio;
    fm_set_operator_freq(s, operator, op_freq);
}

/// Explicitly set the frequency of an oeprator.
void
fm_set_operator_freq(fm* s, fm_operator op, double freq)
{
    _fm* synth = s;
    // TODO
    // synth->op_freq[op] = freq;
    synth->op_step[op] = _calc_step(freq, synth->sample_rate);
}

void
fm_set_operator_strength(fm* s, fm_operator op, double strength)
{
    _fm* synth = s;
    synth->op_strength[op] = strength;
}

inline static double
_cycle_2pi(double value)
{
    if (value > PI2) {
        value = fmod(value, PI2);
    }
    return value;
}

/// Generate n_samples samples in the sample_buf
void
fm_generate_samples(fm* s, int16_t* sample_buf, size_t n_samples)
{
    _fm* synth = s;

    // 1. Compute operator envelopes.
    // 2. Sample the operators, mult with envelopes. Save these.
    // 3. Update: New modulated angles for all the operators.
    // 4. Compute the overall note envelope.
    // 5. Final sample value: Sample the carrier, mult with envelope.
    // 6. Update: Find the base angle of the carrier and modulate it with
    //            operators.
    //
    // 3 steps, all need to be done for carrier and operators:
    // - envelope
    // - sample
    // - update (modulate and step)

    // for now, env will just be the sustain value.

    for (size_t s = 0; s < n_samples; s++) {
        // 1.
        // for (int i = 0; i < FM_OPERATORS; i++) {
        //    TODO Envelopes.
        //}
        double op_samples[FM_OPERATORS];
        double mod_angle = 0.0;
        // 2.
        for (int op = 0; op < FM_OPERATORS; op++) {
            op_samples[op] =
              wt_sample(synth->op_wave[op], synth->op_angle[op]) *
              synth->op_sustain_level[op];
            mod_angle += op_samples[op] * synth->op_strength[op];
        }

        // 3.
        for (int op = 0; op < FM_OPERATORS; op++) {
            // TODO: Connect them together.
            synth->op_angle[op] += synth->op_step[op];
            synth->op_angle[op] = _cycle_2pi(synth->op_angle[op]);
        }

        // 4.
        double env = synth->sustain_level;

        // 5.
        double carrier_sample =
          wt_sample(synth->carrier_wave, synth->carrier_angle);
        int16_t sample_val = round(carrier_sample * env * INT16_MAX);
        sample_buf[s] = sample_val;

        // 6.
        synth->carrier_angle +=
          synth->carrier_step + (synth->carrier_step * mod_angle);
        synth->carrier_angle = _cycle_2pi(synth->carrier_angle);
    }
}
