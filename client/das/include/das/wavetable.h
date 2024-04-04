/**
 * @file wavetable.h
 * @brief Wave tables for generating audio waves.
 * @author Spencer Leslie
 */
#pragma once

#include <math.h>
#include <stdint.h>

/** Number of samples in the tables. */
#define WT_N_SAMPLES 255

/** Types of waves available. */
typedef enum
{
    WAVETYPE_SINE,
    WAVETYPE_SQUARE,
    WAVETYPE_SAW,
} WaveType;

/**
 * @brief Sample the table at the given angle.
 *
 * @param type The table to sample.
 * @param angle The angle to sample at, between 0 and 1.
 * @return double The sample.
 */
double
WaveTable_sample(WaveType type, double angle);
