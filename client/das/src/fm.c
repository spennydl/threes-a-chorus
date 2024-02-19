/**
 * @file fm.c
 * @brief FM Synthesizer Implementation.
 * @author Spencer Leslie
 */
#include "das/fm.h"
#include "das/envelope.h"

#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/** Twelth root of two. Used for computing note frequencies. */
#define TWELVETH_ROOT_OF_TWO 1.059463094359
/** Default sample rate. */
#define SAMPLE_RATE 44100
/** How many times per second to update the ADSR. */
#define ADSR_UPDATES_PER_SEC 32

/** The FM synthesizer.*/
typedef struct
{
    // start with critical section data to ensure alignment
    /** Current operator envelope values. */
    double opEnvelope[FM_OPERATORS];
    /** Current operator sampling angles. */
    double opAngle[FM_OPERATORS];
    /** Current operator angle update step. */
    double opStep[FM_OPERATORS];
    /** Operator modulation matrix. */
    double opModBy[FM_OPERATORS * FM_OPERATORS];
    /** Operator output strength. */
    double opOutput[FM_OPERATORS];
    /** Operator wave type. */
    WaveType opWave[FM_OPERATORS];

    // Everything else is configuration and control
    /** Synthesizer sample rate. */
    size_t sampleRate;
    /** Was a trigger requested? */
    _Atomic int needTrigger;
    /** Was a gate requested? */
    _Atomic int needGate;

    /** The note we are playing. */
    Note note;
    /** The base frequency. */
    double baseFreq;
    /** CM ratio of the operators. */
    double opCM[FM_OPERATORS];

    /** Operator ADSRs. */
    AdsrEnvelope opAdsr[FM_OPERATORS];

} _FmSynth;

/** Calculate the step value for the given frequency given the sample rate.*/
inline static double
_calcStep(double freq, size_t sampleRate);
/** Update the operator frequency. for the current note. */
inline static void
_updateOperatorFreq(_FmSynth* synth, FmOperator op);
/** Update all operator frequencies. */
inline static void
_updateAllOperatorFreq(_FmSynth* synth);

/** Apply the paramters to the synthesizer.*/
static void
_configure(_FmSynth* synth, const FmSynthParams* params, bool initialize);
/** Allocate a new _FmSynth. */
static _FmSynth*
_fmSynthAlloc(void);
/** Set the playing note. */
static void
_setNote(_FmSynth* synth, Note note);
/** Set an operator's CM value.*/
static void
_setOperatorCM(_FmSynth* synth, FmOperator op, double ratio);
/** Trigger all operator's ADSR's. */
static void
_noteOn(_FmSynth* synth);
/** Gate all operator's ADSR's. */
static void
_noteOff(_FmSynth* synth);

inline static double
_calcStep(double freq, size_t sample_rate)
{
    // samples/sec / periods/sec(freq) = samples/period.
    return PI2 / (sample_rate / freq);
}

inline static void
_updateOperatorFreq(_FmSynth* synth, FmOperator op)
{
    double opFreq = synth->baseFreq * synth->opCM[op];
    synth->opStep[op] = _calcStep(opFreq, synth->sampleRate);
}

inline static void
_updateAllOperatorFreq(_FmSynth* synth)
{
    for (int op = 0; op < FM_OPERATORS; op++) {
        _updateOperatorFreq(synth, op);
    }
}

inline static double
cycle2Pi(double value)
{
    while (value < 0) {
        value += PI2;
    }
    if (value > PI2) {
        value = fmod(value, PI2);
    }
    return value;
}

static _FmSynth*
_fmSynthAlloc(void)
{
    _FmSynth* synth = malloc(sizeof(_FmSynth));
    if (synth) {
        memset(synth, 0, sizeof(_FmSynth));
    }
    return synth;
}

static void
_configure(_FmSynth* synth, const FmSynthParams* params, bool initialize)
{
    if (NULL != synth) {

        if (initialize) {
            synth->sampleRate = params->sampleRate;
        }

        _setNote(synth, params->note);

        for (int op = 0; op < FM_OPERATORS; op++) {
            const OperatorParams* opParams = &params->opParams[op];
            synth->opWave[op] = opParams->waveType;

            _setOperatorCM(synth, op, params->opParams[op].CmRatio);
            synth->opEnvelope[op] = 0.00; // TODO: Envelopes.
            synth->opOutput[op] = opParams->outputStrength;

            synth->opAdsr[op].attackMs = opParams->adsr.attackMs;
            synth->opAdsr[op].decayMs = opParams->adsr.decayMs;
            synth->opAdsr[op].releaseMs = opParams->adsr.releaseMs;
            synth->opAdsr[op].attackPeak = opParams->adsr.attackPeak;
            synth->opAdsr[op].sustainLevel = opParams->adsr.sustainLevel;
            // TODO: If we are in the middle of a note we do not want to do
            // this, and we need a way to know that we haven't put ADSR into an
            // invalid state.
            // TODO: something that works whether or not we are updating or
            // intiializing.
            // Need to rethink the ADSR impl.
            if (initialize) {
                Env_adsrInit(
                  &synth->opAdsr[op], ADSR_UPDATES_PER_SEC, synth->sampleRate);
            }

            int modByIdx = op * FM_OPERATORS;
            for (int modOp = 0; modOp < FM_OPERATORS; modOp++) {
                synth->opModBy[modByIdx + modOp] =
                  opParams->algorithmConnections[modOp];
            }
        }
    }
}

static void
_setNote(_FmSynth* synth, Note note)
{
    // Frequency of a note relative to a reference frequency is given by:
    //
    // $$ F * 2^{N / 12} $$
    //
    // where:
    // - F is the reference note frequency
    // - N is how many half-steps away (positive or negative) the target note is
    //   from the reference note.
    synth->baseFreq = A_440 * powf(TWELVETH_ROOT_OF_TWO, note);
    synth->note = note;
    _updateAllOperatorFreq(synth);
}

static void
_setOperatorCM(_FmSynth* synth, FmOperator op, double ratio)
{
    synth->opCM[op] = ratio;
    _updateOperatorFreq(synth, op);
}

static void
_noteOn(_FmSynth* synth)
{
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_adsrTrigger(&synth->opAdsr[op]);
    }
}

static void
_noteOff(_FmSynth* synth)
{
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_adsrGate(&synth->opAdsr[op]);
    }
}

FmSynthesizer*
Fm_defaultSynthesizer(void)
{
    _FmSynth* synth = _fmSynthAlloc();
    if (!synth) {
        return NULL;
    }

    FmSynthesizer* fmSynth = malloc(sizeof(FmSynthesizer));
    if (!fmSynth) {
        free(synth);
        return NULL;
    }
    fmSynth->__FmSynth = synth;

    _configure(synth, &FM_DEFAULT_PARAMS, true);
    return fmSynth;
}

FmSynthesizer*
Fm_createFmSynthesizer(const FmSynthParams* params)
{
    _FmSynth* synth = _fmSynthAlloc();
    if (!synth) {
        return NULL;
    }
    memset(synth, 0, sizeof(_FmSynth));

    FmSynthesizer* fmSynth = malloc(sizeof(FmSynthesizer));
    if (!fmSynth) {
        free(synth);
        return NULL;
    }
    synth->needTrigger = 0;
    synth->needGate = 0;
    fmSynth->__FmSynth = synth;

    _configure(synth, params, true);
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

void
Fm_noteOn(FmSynthesizer* s)
{
    _FmSynth* synth = s->__FmSynth;
    synth->needTrigger = 1;
}

void
Fm_noteOff(FmSynthesizer* s)
{
    _FmSynth* synth = s->__FmSynth;
    synth->needGate = 1;
}

void
Fm_updateParams(FmSynthesizer* s, FmSynthParams* params)
{
    _FmSynth* synth = s->__FmSynth;
    _configure(synth, params, false);
}

void
Fm_generateSamples(FmSynthesizer* s, int16_t* sampleBuf, size_t nSamples)
{
    _FmSynth* synth = s->__FmSynth;

    // I believe stack vars are zered by default?
    double opSamples[FM_OPERATORS] = { 0 };
    double opMod[FM_OPERATORS] = { 0 };
    for (size_t s = 0; s < nSamples; s++) {

        // Compute and cache the current sample values from each
        // operator.
        for (int op = 0; op < FM_OPERATORS; op++) {
            opSamples[op] =
              WaveTable_sample(synth->opWave[op], synth->opAngle[op]) *
              synth->opEnvelope[op];
        }

        // Use the current sample value to modulate the angle of the operators.
        // This will affect the next sample.
        for (int op = 0; op < FM_OPERATORS; op++) {
            int idx = op * FM_OPERATORS;
            for (int modOp = 0; modOp < FM_OPERATORS; modOp++) {
                opMod[op] += synth->opModBy[idx + modOp] * opSamples[modOp];
            }

            synth->opAngle[op] +=
              synth->opStep[op] + (synth->opStep[op] * opMod[op]);

            synth->opAngle[op] = cycle2Pi(synth->opAngle[op]);
        }

        // If we need to, update the ADSR envelope and triggers.
        if (s % ADSR_UPDATES_PER_SEC == 0) {
            // TODO: Maybe there is a better and faster solution for this?
            // Do the trigger first so that if we are in an invalid state where
            // both trigger and gate are set then we can default to off.
            if (synth->needTrigger) {
                _noteOn(synth);
                synth->needTrigger = 0;
            }
            if (synth->needGate) {
                _noteOff(synth);
                synth->needGate = 0;
            }
            for (int op = 0; op < FM_OPERATORS; op++) {
                synth->opEnvelope[op] = Env_adsrUpdate(&synth->opAdsr[op]);
            }
        }

        // Mix together the operators that are wired to output.
        double finalSample = 0;
        for (int op = 0; op < FM_OPERATORS; op++) {
            finalSample += opSamples[op] * synth->opOutput[op];
        }
        sampleBuf[s] = finalSample * INT16_MAX;
    }
}
