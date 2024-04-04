#pragma once

typedef struct
{
    float ptsX[6];
    float ptsY[6];
    int pts;
} Pwl_Function;

#define PWL_ADSR_PLUCK_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 0.1, 0.15, 0.65, 1.0 },                                 \
        .ptsY = { 0.0, 0.65, 0.55, 0.55, 0.0 }, .pts = 5                       \
    }

#define PWL_ADSR_AHH_FUNCTION                                                  \
    {                                                                          \
        .ptsX = { 0.0, 0.25, 0.35, 0.75, 1.0 },                                \
        .ptsY = { 0.0, 0.6, 0.55, 0.55, 0.0 }, .pts = 5                        \
    }

#define PWL_SWELL_FUNCTION                                                     \
    {                                                                          \
        .ptsX = { 0.0, 0.5, 1.0 }, .ptsY = { 0.0, 1.0, 0.0 }, .pts = 3         \
    }

#define PWL_EXP_FALLOFF_FUNCTION                                               \
    {                                                                          \
        .ptsX = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0 }, .ptsY = { 1.0,               \
                                                            0.201896517994655, \
                                                            0.040762203978366, \
                                                            0.008229747049020, \
                                                            0.001661557273174, \
                                                            0.0 },             \
        .pts = 6                                                               \
    }

#define PWL_CONST_FUNCTION                                                     \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 0.6, 0.6 }, .pts = 2                   \
    }

#define PWL_PEAKFALL_FUNCTION                                                  \
    {                                                                          \
        .ptsX = { 0.0, 0.1, 0.2, 0.3, 1.0 },                                   \
        .ptsY = { 0.0, 0.1, 0.18, 0.0, 0.0 }, .pts = 5                         \
    }

#define PWL_LINEARFALL_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 1.0, 0.0 }, .pts = 2                   \
    }

#define PWL_LINEARFALL_P1N1_FUNCTION                                           \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 1.0, -1.0 }, .pts = 2                  \
    }

#define PWL_LINEARRAMP_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 1.0 }, .ptsY = { 0.0, 1.0 }, .pts = 2                   \
    }

#define PWL_BELLSTRIKE_FUNCTION                                                \
    {                                                                          \
        .ptsX = { 0.0, 0.25, 0.75, 1.0 }, .ptsY = { 0.6, 0.35, 0.35, 0.0 },    \
        .pts = 4                                                               \
    }

float
Pwl_sample(Pwl_Function* pwl, float samplePoint);
