#pragma once

#include <stddef.h>

typedef enum
{
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    ADSR_OFF
} _AdsrState;

typedef struct
{
    size_t attackMs;
    size_t decayMs;
    size_t releaseMs;
    double sustainLevel;
    double attackPeak;

    double step;
    double envelope;
    size_t updatesPerSec;

    _AdsrState state;
} AdsrEnvelope;

void
Env_adsrInit(AdsrEnvelope* adsr, size_t updatesPerSecond, size_t sampleRate);

double
Env_adsrUpdate(AdsrEnvelope* adsr);

void
Env_adsrTrigger(AdsrEnvelope* adsr);

void
Env_adsrGate(AdsrEnvelope* adsr);
