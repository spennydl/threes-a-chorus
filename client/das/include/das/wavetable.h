#pragma once

#include <math.h>
#include <stdint.h>

#define WT_N_SAMPLES 256

static const double PI2 = 2.0 * M_PI;

typedef enum
{
    WAVETYPE_SINE,
    WAVETYPE_SQUARE,
    WAVETYPE_SAW,
    // TODO: maybe?
    // WAVETYPE_TRIANGLE
} WaveType;

// This is the new approach.
double
WaveTable_sample(WaveType type, double angle);
