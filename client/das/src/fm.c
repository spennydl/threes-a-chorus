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
#define ADSR_UPDATES_PER_SEC 64

/** The FM synthesizer.*/
typedef struct
{
    // start with critical section data to ensure alignment
    /** Current operator envelope values. */
    float opEnvelope[FM_OPERATORS];
    /** Current operator sampling angles. */
    float opAngle[FM_OPERATORS];
    /** Current operator angle update step. */
    float opStep[FM_OPERATORS];
    /** Operator modulation matrix. */
    float opModBy[FM_OPERATORS * FM_OPERATORS];
    /** Operator output strength. */
    float opOutput[FM_OPERATORS];
    /** Operator wave type. */
    WaveType opWave[FM_OPERATORS];

    // Everything else is configuration and control
    /** Synthesizer sample rate. */
    size_t sampleRate;

    /** The note we are playing. */
    Note note;
    /** The base frequency. */
    float baseFreq;

    float env;
    /** CM ratio of the operators. */
    float opCM[FM_OPERATORS];

    /** Operator ADSRs. */
    Env_Envelope opAdsr[FM_OPERATORS];

    Env_Envelope mainEnvelope;

} _FmSynth;

/** Calculate the step value for the given frequency given the sample rate.*/
inline static float
_calcStep(float freq, size_t sampleRate);
/** Update the operator frequency. for the current note. */
inline static void
_updateOperatorFreq(_FmSynth* synth, FmOperator op);
/** Update all operator frequencies. */
inline static void
_updateAllOperatorFreq(_FmSynth* synth);

/** Apply the paramters to the synthesizer.*/
static void
_configure(_FmSynth* synth, const FmSynthParams* params, bool initialize);
/** Set params for one operator */
static void
_setOpParams(_FmSynth* synth, FmOperator op, const OperatorParams* params);
/** Allocate a new _FmSynth. */
static _FmSynth*
_fmSynthAlloc(void);
/** Set the playing note. */
static void
_setNote(_FmSynth* synth, int note);
/** Set an operator's CM value.*/
static void
_setOperatorCM(_FmSynth* synth, FmOperator op, float ratio, Note fixTo);
/** Trigger all operator's ADSR's. */
static void
_noteOn(_FmSynth* synth);
/** Gate all operator's ADSR's. */
static void
_noteOff(_FmSynth* synth);

inline static float
_calcStep(float freq, size_t sample_rate)
{
    return (freq / sample_rate);
}

inline static void
_updateOperatorFreq(_FmSynth* synth, FmOperator op)
{
    if (synth->opCM[op] > 0) {
        float opFreq = synth->baseFreq * synth->opCM[op];
        synth->opStep[op] = _calcStep(opFreq, synth->sampleRate);
    }
}

inline static void
_updateAllOperatorFreq(_FmSynth* synth)
{
    for (int op = 0; op < FM_OPERATORS; op++) {
        _updateOperatorFreq(synth, op);
    }
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

// TODO: property setting suuuuuucks here.
// This function should only really be used if we're changing voices.
// We need a different approach for tweaking params on the fly.
static void
_configure(_FmSynth* synth, const FmSynthParams* params, bool initialize)
{
    if (NULL != synth) {
        if (initialize) {
            synth->sampleRate = params->sampleRate;
        }

        synth->mainEnvelope = params->envelope;
        Env_prepareEnvelope(&synth->mainEnvelope,
                            synth->sampleRate / ADSR_UPDATES_PER_SEC);

        for (int op = 0; op < FM_OPERATORS; op++) {
            const OperatorParams* opParams = &params->opParams[op];

            _setOpParams(synth, op, opParams);

            // TODO: this sucks
            memcpy(&synth->opAdsr[op],
                   &params->opEnvelopes[op],
                   sizeof(Env_Envelope));

            Env_prepareEnvelope(&synth->opAdsr[op],
                                synth->sampleRate / ADSR_UPDATES_PER_SEC);
        }
    }
}

static void
_setOpParams(_FmSynth* synth, FmOperator op, const OperatorParams* params)
{
    synth->opWave[op] = params->waveType;
    synth->opOutput[op] = params->outputStrength;

    _setOperatorCM(synth, op, params->CmRatio, params->fixToNote);

    int modByIdx = op * FM_OPERATORS;
    for (int modOp = 0; modOp < FM_OPERATORS; modOp++) {
        synth->opModBy[modByIdx + modOp] = params->algorithmConnections[modOp];
    }
}

void
Fm_updateOpParams(FmSynthesizer* s, FmOperator op, const OperatorParams* params)
{
    _FmSynth* synth = s->__FmSynth;
    _setOpParams(synth, op, params);
}

void
Fm_setNote(FmSynthesizer* s, Note note)
{
    _FmSynth* synth = s->__FmSynth;
    _setNote(synth, note);
}

static void
_setNote(_FmSynth* synth, int note) // note is an int to force signness
{
    // Frequency of a note relative to a reference frequency is given by:
    //
    // $$ F * 2^{N / 12} $$
    //
    // where:
    // - F is the reference note frequency
    // - N is how many half-steps away (positive or negative) the target note is
    //   from the reference note.
    synth->baseFreq = C2_HZ * powf(TWELVETH_ROOT_OF_TWO, note);
    synth->note = note;
    _updateAllOperatorFreq(synth);
}

static void
_setOperatorCM(_FmSynth* synth, FmOperator op, float ratio, Note fixTo)
{
    synth->opCM[op] = ratio;
    if (ratio > 0) {
        _updateOperatorFreq(synth, op);
    } else {
        float freq = C2_HZ * powf(TWELVETH_ROOT_OF_TWO, fixTo);
        synth->opStep[op] = _calcStep(freq, synth->sampleRate);
    }
}

static void
_updateEnvelopes(_FmSynth* synth)
{
    synth->env = Env_getValueAndAdvance(&synth->mainEnvelope);
    for (int op = 0; op < FM_OPERATORS; op++) {
        synth->opEnvelope[op] = Env_getValueAndAdvance(&synth->opAdsr[op]);
    }
}

static void
_noteOn(_FmSynth* synth)
{
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_trigger(&synth->opAdsr[op]);
    }
    Env_trigger(&synth->mainEnvelope);
}

static void
_noteOff(_FmSynth* synth)
{
    Env_gate(&synth->mainEnvelope);
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_gate(&synth->opAdsr[op]);
    }
}

FmSynthesizer*
Fm_defaultSynthesizer(void)
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
    _noteOn(synth);
}

void
Fm_noteOff(FmSynthesizer* s)
{
    _FmSynth* synth = s->__FmSynth;
    _noteOff(synth);
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
    float opSamples[FM_OPERATORS] = { 0 };
    float opMod[FM_OPERATORS] = { 0 };
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
            opMod[op] = 0;
            for (int modOp = 0; modOp < FM_OPERATORS; modOp++) {
                opMod[op] += synth->opModBy[idx + modOp] *
                             (opSamples[modOp] * synth->opStep[modOp]); // ??
            }

            synth->opAngle[op] += synth->opStep[op] + opMod[op];
            synth->opAngle[op] -= floorf(synth->opAngle[op]);
        }

        // If we need to, update the ADSR envelope and triggers.
        if (s % ADSR_UPDATES_PER_SEC == 0) {
            _updateEnvelopes(synth);
        }

        // Mix together the operators that are wired to output.
        float finalSample = 0;
        for (int op = 0; op < FM_OPERATORS; op++) {
            finalSample += opSamples[op] * synth->opOutput[op];

            if (finalSample >= 1) {
                fprintf(stderr, "WARN: sample greater than 1!\n");
                finalSample = 1;
            }
        }
        sampleBuf[s] = (synth->env * finalSample) * INT16_MAX;
    }
}
