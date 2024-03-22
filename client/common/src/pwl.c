#include "com/pwl.h"

float
Pwl_sample(Pwl_Function* pwl, float samplePoint)
{
    int i;
    float x0 = 0;
    float x1 = 0;
    float y0 = 0;
    float y1 = 0;

    for (i = 0; i < pwl->pts - 1; i++) {
        if (pwl->ptsX[i] <= samplePoint && pwl->ptsX[i + 1] > samplePoint) {
            x0 = pwl->ptsX[i];
            x1 = pwl->ptsX[i + 1];
            y0 = pwl->ptsY[i];
            y1 = pwl->ptsY[i + 1];
            break;
        }
    }

    float sample = ((samplePoint - x0) / (x1 - x0)) * (y1 - y0) + y0;
    return sample;
}
