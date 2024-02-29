/**
 * @file envelope.c
 * @brief ADSR envelope implementation.
 *
 * I expect this to change at some point.
 *
 * @author Spencer Leslie
 */
#include "das/envelope.h"

/** Transitions */
static void
_adsrTransitionAttack(AdsrEnvelope* adsr)
{
    // depending on the state and time, we need a linear step that will
    // get us to the target level in the target time.
    adsr->envelope = 0.0;
    double samplesToTarget = adsr->updatesPerSec * (adsr->attackMs / 1000.0);
    // TODO: precision is gonna be awful here.
    adsr->step = adsr->attackPeak / samplesToTarget;
    adsr->state = ATTACK;
}

static void
_adsrTransitionDecay(AdsrEnvelope* adsr)
{
    // we're assuming we're at the attack peak here
    double samplesToTarget = adsr->updatesPerSec * (adsr->decayMs / 1000.0);
    // TODO: precision is gonna be awful here.
    // Note that this could decay "upwards" if we start below sustain
    adsr->step = -1 * (adsr->attackPeak - adsr->sustainLevel) / samplesToTarget;
    adsr->state = DECAY;
}

static void
_adsrTransitionSustain(AdsrEnvelope* adsr)
{
    adsr->step = 0.0;
    adsr->envelope = adsr->sustainLevel;
    adsr->state = SUSTAIN;
}

static void
_adsrTransitionOff(AdsrEnvelope* adsr)
{
    adsr->step = 0.0;
    adsr->envelope = 0.0;
    adsr->state = ADSR_OFF;
}

static void
_adsrTransitionRelease(AdsrEnvelope* adsr)
{
    // depending on the state and time, we need a linear step that will
    // get us to the target level in the target time.
    double samplesToTarget = adsr->updatesPerSec * (adsr->releaseMs / 1000.0);
    // TODO: precision could be awful here, but it might not matter
    adsr->step = -1 * adsr->envelope / samplesToTarget;
    adsr->state = RELEASE;
}

static void
_adsrTransitionIfNeeded(AdsrEnvelope* adsr)
{
    switch (adsr->state) {
        case ATTACK: {
            if (adsr->envelope >= adsr->attackPeak) {
                _adsrTransitionDecay(adsr);
            }
            break;
        }
        case DECAY: {
            if (adsr->envelope <= adsr->sustainLevel) { // TODO: assumption!
                _adsrTransitionSustain(adsr);
            }
            break;
        }
        case RELEASE: {
            if (adsr->envelope <= 0) {
                _adsrTransitionOff(adsr);
            }
            break;
        }
        default: {
            // Transitions to attack and sustain must be manually triggered
            // using Env_adsrTrigger and Env_adsrGate.
            break;
        }
    }
}

void
Env_adsrInit(AdsrEnvelope* adsr, size_t updatesPerSec, size_t sampleRate)
{
    adsr->updatesPerSec = (sampleRate / updatesPerSec);
    adsr->state = ADSR_OFF;
    adsr->step = 0.0;
    adsr->envelope = 0.0;

    // Enforce that the attack goes at least as high as the sustain level.
    if (adsr->attackPeak < adsr->sustainLevel) {
        adsr->attackPeak = adsr->sustainLevel;
    }
}

double
Env_adsrUpdate(AdsrEnvelope* adsr)
{
    double ret = 0.0;
    if (adsr->state != ADSR_OFF) {
        ret = adsr->envelope;
        adsr->envelope += adsr->step;
        _adsrTransitionIfNeeded(adsr);
    }

    return ret;
}

void
Env_adsrTrigger(AdsrEnvelope* adsr)
{
    _adsrTransitionAttack(adsr);
}

void
Env_adsrGate(AdsrEnvelope* adsr)
{
    _adsrTransitionRelease(adsr);
}
