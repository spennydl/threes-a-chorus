#include "das/fm.h"
#include "das/envelope.h"

#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TWELVETH_ROOT_OF_TWO 1.059463094359
#define SAMPLE_RATE 44100

#define ADSR_UPDATES_PER_SEC 64

typedef struct
{
    Note note;
    WaveType carrierWave;
    size_t sampleRate;
    size_t attackMs;
    size_t decayMs;
    size_t releaseMs;

    AdsrEnvelope adsr;

    double envelope;
    double carrierFreq;
    double carrierAngle;
    double carrierStep;

    double opEnvelope[FM_OPERATORS];
    double opAngle[FM_OPERATORS];
    double opStep[FM_OPERATORS];
    double opCM[FM_OPERATORS];
    double opStrength[FM_OPERATORS];

    WaveType opWave[FM_OPERATORS];
    double opModBy[FM_OPERATORS * FM_OPERATORS];

    AdsrEnvelope opAdsr[FM_OPERATORS];
} _FmSynth;

inline static double
calcStep(double freq, size_t sampleRate);
static void
configure(FmSynthesizer* synth, const FmSynthParams* params);
static _FmSynth*
fmSynthAlloc(void);

static _FmSynth*
fmSynthAlloc(void)
{
    _FmSynth* synth = malloc(sizeof(_FmSynth));
    if (synth) {
        memset(synth, 0, sizeof(_FmSynth));
    }
    return synth;
}

inline static double
calcStep(double freq, size_t sample_rate)
{
    // samples/sec / periods/sec(freq) = samples/period.
    return PI2 / (sample_rate / freq);
}

static void
configure(FmSynthesizer* syn, const FmSynthParams* params)
{
    _FmSynth* synth = syn->__FmSynth;
    if (NULL != synth) {
        Fm_setNote(syn, params->note);
        synth->sampleRate = params->sampleRate;
        synth->carrierStep = calcStep(synth->carrierFreq, synth->sampleRate);
        synth->envelope = 0.0;
        synth->carrierWave = params->carrierWaveType;

        synth->adsr.attackMs = 300;
        synth->adsr.decayMs = 150;
        synth->adsr.releaseMs = 300;
        synth->adsr.attackPeak = 0.75;
        synth->adsr.sustainLevel = 0.55;
        Env_adsrInit(&synth->adsr, ADSR_UPDATES_PER_SEC, synth->sampleRate);

        for (int op = 0; op < FM_OPERATORS; op++) {
            const OperatorParams* opParams = &params->opParams[op];
            synth->opWave[op] = opParams->waveType;
            synth->opStrength[op] = opParams->strength;

            Fm_setOperatorCM(syn, op, opParams->C, opParams->M);
            // TODO: create a default adsr
            synth->opEnvelope[op] = 0.00; // TODO: Envelopes.

            synth->opAdsr[op].attackMs = 500;
            synth->opAdsr[op].decayMs = 100;
            synth->opAdsr[op].releaseMs = 500;
            synth->opAdsr[op].attackPeak = 1.0;
            synth->opAdsr[op].sustainLevel = 0.85;
            Env_adsrInit(
              &synth->opAdsr[op], ADSR_UPDATES_PER_SEC, synth->sampleRate);
        }
    }
}

FmSynthesizer*
fm_defaultSynthesizer(void)
{
    _FmSynth* synth = fmSynthAlloc();
    if (!synth) {
        return NULL;
    }

    FmSynthesizer* fmSynth = malloc(sizeof(FmSynthesizer));
    if (!fmSynth) {
        free(synth);
        return NULL;
    }
    fmSynth->__FmSynth = synth;

    configure(fmSynth, &FM_DEFAULT_PARAMS);
    return fmSynth;
}

FmSynthesizer*
Fm_createFmSynthesizer(const FmSynthParams* params)
{
    _FmSynth* synth = fmSynthAlloc();
    if (!synth) {
        return NULL;
    }

    FmSynthesizer* fmSynth = malloc(sizeof(FmSynthesizer));
    if (!fmSynth) {
        free(synth);
        return NULL;
    }
    fmSynth->__FmSynth = synth;

    configure(fmSynth, params);
    return fmSynth;
}

void
Fm_destroySynthesizer(FmSynthesizer* synth)
{
    if (NULL != synth) {
        if (NULL != synth->__FmSynth) {
            free(synth->__FmSynth);
        }
        free(synth);
    }
}

/// Connect an operator to another
void
Fm_connectOperators(FmSynthesizer* s, FmOperator from, FmOperator to)
{
    // TODO implement
    (void)s;
    (void)from;
    (void)to;
}

/// Set the carrier note
void
Fm_setNote(FmSynthesizer* s, Note note)
{
    _FmSynth* synth = s->__FmSynth;
    // Frequency of a note relative to a reference frequency is given by:
    //
    // $$ F * 2^{N / 12} $$
    //
    // where:
    // - F is the reference note frequency
    // - N is how many half-steps away (positive or negative) the target note is
    //   from the reference note.
    synth->carrierFreq = A_440 * powf(TWELVETH_ROOT_OF_TWO, note);
    synth->carrierStep = calcStep(synth->carrierFreq, synth->sampleRate);
    synth->note = note;

    for (int op = 0; op < FM_OPERATORS; op++) {
        Fm_setOpFrequency(s, op, synth->carrierFreq * synth->opCM[op]);
    }
}

// TODO: Do we need to continue to support both specific operator frequencies as
// well as ratios?
//
// Either way this will need some cleaning up.
void
Fm_setOperatorCM(FmSynthesizer* s,
                 FmOperator
                 operator,
                 unsigned char C,
                 unsigned char M)
{
    _FmSynth* synth = s->__FmSynth;
    double ratio = (double)C / (double)M;
    synth->opCM[operator] = ratio;
    double op_freq = synth->carrierFreq * ratio;
    Fm_setOpFrequency(s, operator, op_freq);
}

void
Fm_noteOn(FmSynthesizer* s)
{
    _FmSynth* synth = s->__FmSynth;
    Env_adsrTrigger(&synth->adsr);
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_adsrTrigger(&synth->opAdsr[op]);
    }
}

void
Fm_noteOff(FmSynthesizer* s)
{
    _FmSynth* synth = s->__FmSynth;
    Env_adsrGate(&synth->adsr);
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_adsrGate(&synth->opAdsr[op]);
    }
}

/// Explicitly set the frequency of an oeprator.
void
Fm_setOpFrequency(FmSynthesizer* s, FmOperator op, double freq)
{
    _FmSynth* synth = s->__FmSynth;
    // TODO
    // synth->op_freq[op] = freq;
    synth->opStep[op] = calcStep(freq, synth->sampleRate);
}

void
Fm_setOpStrength(FmSynthesizer* s, FmOperator op, double strength)
{
    _FmSynth* synth = s->__FmSynth;
    synth->opStrength[op] = strength;
}

inline static double
cycle2Pi(double value)
{
    if (value > PI2) {
        value = fmod(value, PI2);
    }
    return value;
}

/// Generate n_samples samples in the sample_buf
void
Fm_generateSamples(FmSynthesizer* s, int16_t* sampleBuf, size_t nSamples)
{
    _FmSynth* synth = s->__FmSynth;

    // 1. Compute operator envelopes. (do at end now)
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

    for (size_t s = 0; s < nSamples; s++) {
        double opSamples[FM_OPERATORS];
        double angle = 0.0;
        // 2.
        for (int op = 0; op < FM_OPERATORS; op++) {
            opSamples[op] =
              WaveTable_sample(synth->opWave[op], synth->opAngle[op]) *
              synth->opEnvelope[op];
            angle += opSamples[op] * synth->opStrength[op];
        }

        // 3.
        for (int op = 0; op < FM_OPERATORS; op++) {
            // TODO: Connect them together.
            synth->opAngle[op] += synth->opStep[op];
            synth->opAngle[op] = cycle2Pi(synth->opAngle[op]);
        }

        // synth->envelope = synth->adsr.envelope;
        //   for (int op = 0; op < FM_OPERATORS; op++) {
        //   adsrUpdate(&synth->opAdsr[op], synth->sampleRate);
        //   synth->opEnvelope[op] = synth->opAdsr[op].envelope;
        //  }

        // 5.
        double carrierSample =
          WaveTable_sample(synth->carrierWave, synth->carrierAngle);
        int16_t sampleVal = round(carrierSample * synth->envelope * INT16_MAX);
        sampleBuf[s] = sampleVal;

        // 4.
        // But not every frame!
        if (s % ADSR_UPDATES_PER_SEC == 0) {
            synth->envelope = Env_adsrUpdate(&synth->adsr);
            for (int op = 0; op < FM_OPERATORS; op++) {
                synth->opEnvelope[op] = Env_adsrUpdate(&synth->opAdsr[op]);
            }
        }

        // 6.
        synth->carrierAngle +=
          synth->carrierStep + (synth->carrierStep * angle);
        synth->carrierAngle = cycle2Pi(synth->carrierAngle);
    }
}
