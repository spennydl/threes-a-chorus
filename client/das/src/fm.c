#include "das/fm.h"
#include "das/envelope.h"

#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TWELVETH_ROOT_OF_TWO 1.059463094359
#define SAMPLE_RATE 44100

#define ADSR_UPDATES_PER_SEC 32

typedef enum
{
    UPDATE_NOTE,
    UPDATE_NOTE_ON,
    UPDATE_NOTE_OFF,
    UPDATE_OP_STRENGTH,
    UPDATE_OP_CM
} _FmParamUpdateType;

typedef struct _FmUpdate
{
    _FmParamUpdateType type;
    struct _FmUpdate* next;
    FmOperator op;
    union
    {
        Note noteVal;
        double doubleVal;
        struct
        {
            unsigned char C;
            unsigned char M;
        } CmVal;
    };
} _FmParamUpdate;

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

    _FmParamUpdate* updates;
    pthread_mutex_t updateMutex;
} _FmSynth;

inline static double
calcStep(double freq, size_t sampleRate);
static void
configure(FmSynthesizer* synth, const FmSynthParams* params);
static _FmSynth*
fmSynthAlloc(void);

/// Set the carrier note
static void
_setNote(_FmSynth* synth, Note note);

static void
_setOpFrequency(_FmSynth* synth, FmOperator op, double freq);

static void
_setOperatorCM(_FmSynth* synth,
               FmOperator op,
               unsigned char C,
               unsigned char M);

static void
_setOpStrength(_FmSynth* synth, FmOperator op, double strength);

static void
_noteOn(_FmSynth* synth);
static void
_noteOff(_FmSynth* synth);

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
        pthread_mutex_init(&synth->updateMutex, NULL);

        _setNote(synth, params->note);
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

        synth->updates = NULL;

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
Fm_defaultSynthesizer(void)
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
        _FmSynth* s = synth->__FmSynth;
        pthread_mutex_destroy(&s->updateMutex);
        // TODO we're leaking the queue
        if (NULL != synth->__FmSynth) {
            free(synth->__FmSynth);
        }
        free(synth);
    }
}

/// Connect an operator to another
void
Fm_connectOperators(FmSynthesizer* s,
                    FmOperator from,
                    FmOperator to,
                    double strength)
{
    _FmSynth* synth = s->__FmSynth;
    size_t idx = (to * FM_OPERATORS) + from;
    synth->opModBy[idx] = strength;
}

/// Set the carrier note
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
    synth->carrierFreq = A_440 * powf(TWELVETH_ROOT_OF_TWO, note);
    synth->carrierStep = calcStep(synth->carrierFreq, synth->sampleRate);
    synth->note = note;

    for (int op = 0; op < FM_OPERATORS; op++) {
        _setOpFrequency(synth, op, synth->carrierFreq * synth->opCM[op]);
    }
}

static void
_addUpdateToQueue(_FmSynth* synth, _FmParamUpdate* update)
{
    pthread_mutex_lock(&synth->updateMutex);
    _FmParamUpdate** head = &synth->updates;
    while (*head != NULL) {
        *head = (*head)->next;
    }
    *head = update;
    pthread_mutex_unlock(&synth->updateMutex);
}

void
Fm_setNote(FmSynthesizer* s, Note note)
{
    _FmSynth* synth = s->__FmSynth;

    _FmParamUpdate* update = malloc(sizeof(_FmParamUpdate));
    update->next = NULL;
    update->type = UPDATE_NOTE;
    update->noteVal = note;

    _addUpdateToQueue(synth, update);
}

void
Fm_setOperatorCM(FmSynthesizer* s,
                 FmOperator
                 operator,
                 unsigned char C,
                 unsigned char M)
{
    _FmSynth* synth = s->__FmSynth;
    _FmParamUpdate* update = malloc(sizeof(_FmParamUpdate));
    update->next = NULL;

    update->CmVal.C = C;
    update->CmVal.M = M;
    update->op = operator;

    _addUpdateToQueue(synth, update);
}

void
Fm_performQueuedUpdates(FmSynthesizer* s)
{
    _FmSynth* synth = s->__FmSynth;

    pthread_mutex_lock(&synth->updateMutex);

    _FmParamUpdate* updateHead = synth->updates;
    while (updateHead != NULL) {
        switch (updateHead->type) {
            case UPDATE_NOTE: {
                _setNote(synth, updateHead->noteVal);
                break;
            }
            case UPDATE_OP_STRENGTH: {
                _setOpStrength(synth, updateHead->op, updateHead->doubleVal);
                break;
            } break;
            case UPDATE_OP_CM: {
                _setOperatorCM(synth,
                               updateHead->op,
                               updateHead->CmVal.C,
                               updateHead->CmVal.M);
                break;
            }
            case UPDATE_NOTE_ON: {
                _noteOn(synth);
                break;
            }
            case UPDATE_NOTE_OFF: {
                _noteOff(synth);
                break;
            }
            default: {
                // TODO: This shouldnt happen but we should do something?
                break;
            }
        }
        _FmParamUpdate* next = updateHead->next;
        free(updateHead);
        updateHead = next;
    }
    synth->updates = updateHead; // should be NULL now

    pthread_mutex_unlock(&synth->updateMutex);
}

static void
_setOperatorCM(_FmSynth* synth, FmOperator op, unsigned char C, unsigned char M)
{
    double ratio = (double)C / (double)M;
    synth->opCM[op] = ratio;
    double op_freq = synth->carrierFreq * ratio;
    _setOpFrequency(synth, op, op_freq);
}

static void
_noteOn(_FmSynth* synth)
{
    Env_adsrTrigger(&synth->adsr);
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_adsrTrigger(&synth->opAdsr[op]);
    }
}

void
Fm_noteOn(FmSynthesizer* s)
{
    _FmParamUpdate* update = malloc(sizeof(_FmParamUpdate));
    update->next = NULL;
    _FmSynth* synth = s->__FmSynth;
    update->type = UPDATE_NOTE_ON;
    _addUpdateToQueue(synth, update);
}

static void
_noteOff(_FmSynth* synth)
{
    Env_adsrGate(&synth->adsr);
    for (int op = 0; op < FM_OPERATORS; op++) {
        Env_adsrGate(&synth->opAdsr[op]);
    }
}

void
Fm_noteOff(FmSynthesizer* s)
{
    _FmParamUpdate* update = malloc(sizeof(_FmParamUpdate));
    update->next = NULL;
    _FmSynth* synth = s->__FmSynth;
    update->type = UPDATE_NOTE_OFF;
    _addUpdateToQueue(synth, update);
}
/// Explicitly set the frequency of an oeprator.
static void
_setOpFrequency(_FmSynth* synth, FmOperator op, double freq)
{
    synth->opStep[op] = calcStep(freq, synth->sampleRate);
}

static void
_setOpStrength(_FmSynth* synth, FmOperator op, double strength)
{
    synth->opStrength[op] = strength;
}

void
Fm_setOpStrength(FmSynthesizer* s, FmOperator op, double strength)
{
    _FmParamUpdate* update = malloc(sizeof(_FmParamUpdate));
    update->next = NULL;

    _FmSynth* synth = s->__FmSynth;
    update->type = UPDATE_OP_STRENGTH;
    update->op = op;
    update->doubleVal = strength;

    _addUpdateToQueue(synth, update);
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
            int idx = op * FM_OPERATORS;
            double opMod = 0;
            for (int modOp = 0; modOp < FM_OPERATORS; modOp++) {
                opMod += synth->opModBy[idx + modOp] * opSamples[modOp];
            }
            synth->opAngle[op] +=
              synth->opStep[op] + (synth->opStep[op] * opMod);
            synth->opAngle[op] = cycle2Pi(synth->opAngle[op]);
        }

        // 5.
        double carrierSample =
          WaveTable_sample(synth->carrierWave, synth->carrierAngle);
        int16_t sampleVal = round(carrierSample * synth->envelope * INT16_MAX);
        sampleBuf[s] = sampleVal;

        // 4.
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
