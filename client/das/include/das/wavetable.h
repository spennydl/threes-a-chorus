#ifndef TAC_DAS_WAVETABLE_H
#define TAC_DAS_WAVETABLE_H

#include <math.h>
#include <stdint.h>

// 101 samples. At 44100hz, A400 will be sampled with 100.... samples, so
// let's try sampling a sine wave with 101 samples. I think we can get a couple
// octaves without badly aliasing this way.
#define WT_N_SAMPLES 256

static const double PI2 = 2.0 * M_PI;

typedef enum
{
    WAVETYPE_SINE,
    WAVETYPE_SQUARE,
    WAVETYPE_SAW,
    WAVETYPE_TRIANGLE
} wave_type;

// This is the new approach.
double
wt_sample(wave_type type, double phase_angle);

#endif // TAC_DAS_WAVETABLE_H
