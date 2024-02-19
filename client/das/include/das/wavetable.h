/**
 * @file wavetable.h
 * @brief Wave tables for generating audio waves.
 * @author Spencer Leslie
 */
#pragma once

#include <math.h>
#include <stdint.h>

/** Number of samples in the tables. */
#define WT_N_SAMPLES 256

/** 2pi. */
static const double PI2 = 2.0 * M_PI;

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
 * @param angle The angle to sample at.
 * @return double The sample.
 */
double
WaveTable_sample(WaveType type, double angle);
