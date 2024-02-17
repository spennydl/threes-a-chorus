#include "das/fm.h"

#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TWELVETH_ROOT_OF_TWO 1.059463094359
#define SAMPLE_RATE 44100

typedef enum
{
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    ADSR_OFF
} AdsrState;

typedef struct
{
    size_t attackMs;
    size_t decayMs;
    size_t releaseMs;
    double sustainLevel;
    double attackPeak;

    AdsrState state;
    double step;
    double envelope;
} Adsr;

static void
adsrInit(Adsr* adsr)
{
    adsr->state = ADSR_OFF;
    adsr->step = 0.0;
    adsr->envelope = 0.0;
}

static void
adsrTransitionAttack(Adsr* adsr, size_t sampleRate)
{
    // depending on the state and time, we need a linear step that will
    // get us to the target level in the target time.
    adsr->envelope = 0.0;
    double samplesToTarget = sampleRate * (adsr->attackMs / 1000.0);
    // TODO: precision is gonna be awful here.
    adsr->step = adsr->attackPeak / samplesToTarget;
    adsr->state = ATTACK;
}

static void
adsrTransitionDecay(Adsr* adsr, size_t sampleRate)
{
    // we're assuming we're at the attack peak here
    double samplesToTarget = sampleRate * (adsr->decayMs / 1000.0);
    // TODO: precision is gonna be awful here.
    // Note that this could decay "upwards" if we start below sustain
    adsr->step = -1 * (adsr->attackPeak - adsr->sustainLevel) / samplesToTarget;
    adsr->state = DECAY;
}

static void
adsrTransitionSustain(Adsr* adsr)
{
    adsr->step = 0.0;
    adsr->envelope = adsr->sustainLevel;
    adsr->state = SUSTAIN;
}

static void
adsrTransitionOff(Adsr* adsr)
{
    adsr->step = 0.0;
    adsr->envelope = 0.0;
    adsr->state = ADSR_OFF;
}

static void
adsrTransitionRelease(Adsr* adsr, size_t sampleRate)
{
    // depending on the state and time, we need a linear step that will
    // get us to the target level in the target time.
    double samplesToTarget = sampleRate * (adsr->releaseMs / 1000.0);
    // TODO: precision is gonna be awful here.
    adsr->step = -1 * adsr->envelope / samplesToTarget;
    adsr->state = RELEASE;
}

static void
adsrTransitionIfNeeded(Adsr* adsr, size_t sampleRate)
{
    switch (adsr->state) {
        case ATTACK: {
            if (adsr->envelope >= adsr->attackPeak) {
                fprintf(stderr, "TRANSITION ATTACK %f\n", adsr->envelope);
                adsrTransitionDecay(adsr, sampleRate);
            }
            break;
        }
        case DECAY: {
            if (adsr->envelope <= adsr->sustainLevel) { // TODO: assumption!
                fprintf(stderr, "TRANSITION DECAY %f\n", adsr->envelope);
                adsrTransitionSustain(adsr);
            }
            break;
        }
        case RELEASE: {
            if (adsr->envelope <= 0) {
                fprintf(stderr, "TRANSITION RELEASE %f\n", adsr->envelope);
                adsrTransitionOff(adsr);
            }
            break;
        }
        default: {
            // Transitions to attack and sustain must be manually triggered
            break;
        }
    }
}

// static void
// adsrGate(Adsr* adsr, size_t sampleRate)
//{
// if (adsr->state == SUSTAIN) {
// adsrTransitionRelease(adsr, sampleRate);
//} else {
// adsrTransitionAttack(adsr, sampleRate);
//}
//}
//
static double
adsrUpdate(Adsr* adsr, size_t sampleRate)
{
    double ret = 0.0;
    if (adsr->state != ADSR_OFF) {
        ret = adsr->envelope;
        adsr->envelope += adsr->step;
        adsrTransitionIfNeeded(adsr, sampleRate);
    }

    return ret;
}

typedef struct
{
    Note note;
    WaveType carrierWave;
    size_t sampleRate;
    size_t attackMs;
    size_t decayMs;
    size_t releaseMs;

    Adsr adsr;

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

    Adsr opAdsr[FM_OPERATORS];
} FmSynth;

inline static double
calcStep(double freq, size_t sampleRate);
static void
configure(FmSynthesizer* synth, const FmSynthParams* params);
static FmSynth*
fmSynthAlloc(void);

static FmSynth*
fmSynthAlloc(void)
{
    FmSynth* synth = malloc(sizeof(FmSynth));
    if (synth) {
        memset(synth, 0, sizeof(FmSynth));
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
    FmSynth* synth = syn->__FmSynth;
    if (NULL != synth) {
        Fm_setNote(syn, params->note);
        synth->sampleRate = params->sampleRate;
        synth->carrierStep = calcStep(synth->carrierFreq, synth->sampleRate);
        synth->envelope = 0.0;
        synth->carrierWave = params->carrierWaveType;

        adsrInit(&synth->adsr);
        synth->adsr.attackMs = 300;
        synth->adsr.decayMs = 150;
        synth->adsr.releaseMs = 100;
        synth->adsr.attackPeak = 1.0;
        synth->adsr.sustainLevel = 0.55;

        for (int op = 0; op < FM_OPERATORS; op++) {
            const OperatorParams* opParams = &params->opParams[op];
            synth->opWave[op] = opParams->waveType;
            synth->opStrength[op] = opParams->strength;

            Fm_setOperatorCM(syn, op, opParams->C, opParams->M);
            // TODO: create a default adsr
            synth->opEnvelope[op] = 0.75; // TODO: Envelopes.
        }
    }
}

FmSynthesizer*
fm_defaultSynthesizer(void)
{
    FmSynth* synth = fmSynthAlloc();
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
    FmSynth* synth = fmSynthAlloc();
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
    FmSynth* synth = s->__FmSynth;
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
    FmSynth* synth = s->__FmSynth;
    double ratio = (double)C / (double)M;
    synth->opCM[operator] = ratio;
    double op_freq = synth->carrierFreq * ratio;
    Fm_setOpFrequency(s, operator, op_freq);
}

void
Fm_triggerNote(FmSynthesizer* s)
{
    FmSynth* synth = s->__FmSynth;
    adsrTransitionAttack(&synth->adsr, synth->sampleRate);
}

void
Fm_gateNote(FmSynthesizer* s)
{
    FmSynth* synth = s->__FmSynth;
    adsrTransitionRelease(&synth->adsr, synth->sampleRate);
}

/// Explicitly set the frequency of an oeprator.
void
Fm_setOpFrequency(FmSynthesizer* s, FmOperator op, double freq)
{
    FmSynth* synth = s->__FmSynth;
    // TODO
    // synth->op_freq[op] = freq;
    synth->opStep[op] = calcStep(freq, synth->sampleRate);
}

void
Fm_setOpStrength(FmSynthesizer* s, FmOperator op, double strength)
{
    FmSynth* synth = s->__FmSynth;
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
    FmSynth* synth = s->__FmSynth;

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

    for (size_t s = 0; s < nSamples; s++) {
        // 1.
        // for (int i = 0; i < FM_OPERATORS; i++) {
        //    TODO Envelopes.
        //}
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

        // 4.
        // But not every frame!
        adsrUpdate(&synth->adsr, synth->sampleRate);
        synth->envelope = synth->adsr.envelope;
        //  for (int op = 0; op < FM_OPERATORS; op++) {
        //  adsrUpdate(&synth->opAdsr[op], synth->sampleRate);
        //  synth->opEnvelope[op] = synth->opAdsr[op].envelope;
        // }

        // 5.
        double carrierSample =
          WaveTable_sample(synth->carrierWave, synth->carrierAngle);
        int16_t sampleVal = round(carrierSample * synth->envelope * INT16_MAX);
        sampleBuf[s] = sampleVal;

        // 6.
        synth->carrierAngle +=
          synth->carrierStep + (synth->carrierStep * angle);
        synth->carrierAngle = cycle2Pi(synth->carrierAngle);
    }
}
