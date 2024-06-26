/**
 * @file wavetable.c
 * @brief Implementation of the wave table.
 * @author Spencer Leslie 301571329
 */
#include "das/wavetable.h"
#include <malloc.h>
#include <math.h>
#include <stdbool.h>

/** 255 samples of one period of a sine wave. */
static const double sineTable[] = { 0,
                                    0.024541228522912,
                                    0.049067674327418,
                                    0.073564563599667,
                                    0.098017140329561,
                                    0.122410675199216,
                                    0.146730474455362,
                                    0.170961888760301,
                                    0.195090322016128,
                                    0.219101240156870,
                                    0.242980179903264,
                                    0.266712757474898,
                                    0.290284677254462,
                                    0.313681740398892,
                                    0.336889853392220,
                                    0.359895036534988,
                                    0.382683432365090,
                                    0.405241314004990,
                                    0.427555093430282,
                                    0.449611329654607,
                                    0.471396736825998,
                                    0.492898192229784,
                                    0.514102744193222,
                                    0.534997619887097,
                                    0.555570233019602,
                                    0.575808191417845,
                                    0.595699304492433,
                                    0.615231590580627,
                                    0.634393284163645,
                                    0.653172842953777,
                                    0.671558954847018,
                                    0.689540544737067,
                                    0.707106781186547,
                                    0.724247082951467,
                                    0.740951125354959,
                                    0.757208846506485,
                                    0.773010453362737,
                                    0.788346427626606,
                                    0.803207531480645,
                                    0.817584813151584,
                                    0.831469612302545,
                                    0.844853565249707,
                                    0.857728610000272,
                                    0.870086991108711,
                                    0.881921264348355,
                                    0.893224301195515,
                                    0.903989293123443,
                                    0.914209755703531,
                                    0.923879532511287,
                                    0.932992798834739,
                                    0.941544065183021,
                                    0.949528180593037,
                                    0.956940335732209,
                                    0.963776065795440,
                                    0.970031253194544,
                                    0.975702130038529,
                                    0.980785280403230,
                                    0.985277642388941,
                                    0.989176509964781,
                                    0.992479534598710,
                                    0.995184726672197,
                                    0.997290456678690,
                                    0.998795456205172,
                                    0.999698818696204,
                                    1.000000000000000,
                                    0.999698818696204,
                                    0.998795456205172,
                                    0.997290456678690,
                                    0.995184726672197,
                                    0.992479534598710,
                                    0.989176509964781,
                                    0.985277642388941,
                                    0.980785280403230,
                                    0.975702130038529,
                                    0.970031253194544,
                                    0.963776065795440,
                                    0.956940335732209,
                                    0.949528180593037,
                                    0.941544065183021,
                                    0.932992798834739,
                                    0.923879532511287,
                                    0.914209755703531,
                                    0.903989293123443,
                                    0.893224301195515,
                                    0.881921264348355,
                                    0.870086991108711,
                                    0.857728610000272,
                                    0.844853565249707,
                                    0.831469612302545,
                                    0.817584813151584,
                                    0.803207531480645,
                                    0.788346427626606,
                                    0.773010453362737,
                                    0.757208846506485,
                                    0.740951125354959,
                                    0.724247082951467,
                                    0.707106781186548,
                                    0.689540544737067,
                                    0.671558954847019,
                                    0.653172842953777,
                                    0.634393284163645,
                                    0.615231590580627,
                                    0.595699304492433,
                                    0.575808191417845,
                                    0.555570233019602,
                                    0.534997619887097,
                                    0.514102744193222,
                                    0.492898192229784,
                                    0.471396736825998,
                                    0.449611329654607,
                                    0.427555093430282,
                                    0.405241314004990,
                                    0.382683432365090,
                                    0.359895036534988,
                                    0.336889853392220,
                                    0.313681740398891,
                                    0.290284677254462,
                                    0.266712757474898,
                                    0.242980179903264,
                                    0.219101240156870,
                                    0.195090322016129,
                                    0.170961888760301,
                                    0.146730474455362,
                                    0.122410675199216,
                                    0.098017140329561,
                                    0.073564563599668,
                                    0.049067674327418,
                                    0.024541228522912,
                                    0.000000000000000,
                                    -0.024541228522912,
                                    -0.049067674327418,
                                    -0.073564563599667,
                                    -0.098017140329561,
                                    -0.122410675199216,
                                    -0.146730474455362,
                                    -0.170961888760301,
                                    -0.195090322016128,
                                    -0.219101240156870,
                                    -0.242980179903264,
                                    -0.266712757474898,
                                    -0.290284677254462,
                                    -0.313681740398891,
                                    -0.336889853392220,
                                    -0.359895036534988,
                                    -0.382683432365090,
                                    -0.405241314004990,
                                    -0.427555093430282,
                                    -0.449611329654607,
                                    -0.471396736825998,
                                    -0.492898192229784,
                                    -0.514102744193222,
                                    -0.534997619887097,
                                    -0.555570233019602,
                                    -0.575808191417845,
                                    -0.595699304492433,
                                    -0.615231590580627,
                                    -0.634393284163645,
                                    -0.653172842953777,
                                    -0.671558954847018,
                                    -0.689540544737067,
                                    -0.707106781186547,
                                    -0.724247082951467,
                                    -0.740951125354959,
                                    -0.757208846506484,
                                    -0.773010453362737,
                                    -0.788346427626606,
                                    -0.803207531480645,
                                    -0.817584813151584,
                                    -0.831469612302545,
                                    -0.844853565249707,
                                    -0.857728610000272,
                                    -0.870086991108711,
                                    -0.881921264348355,
                                    -0.893224301195515,
                                    -0.903989293123443,
                                    -0.914209755703530,
                                    -0.923879532511287,
                                    -0.932992798834739,
                                    -0.941544065183021,
                                    -0.949528180593037,
                                    -0.956940335732209,
                                    -0.963776065795440,
                                    -0.970031253194544,
                                    -0.975702130038528,
                                    -0.980785280403230,
                                    -0.985277642388941,
                                    -0.989176509964781,
                                    -0.992479534598710,
                                    -0.995184726672197,
                                    -0.997290456678690,
                                    -0.998795456205172,
                                    -0.999698818696204,
                                    -1.000000000000000,
                                    -0.999698818696204,
                                    -0.998795456205172,
                                    -0.997290456678690,
                                    -0.995184726672197,
                                    -0.992479534598710,
                                    -0.989176509964781,
                                    -0.985277642388941,
                                    -0.980785280403230,
                                    -0.975702130038529,
                                    -0.970031253194544,
                                    -0.963776065795440,
                                    -0.956940335732209,
                                    -0.949528180593037,
                                    -0.941544065183021,
                                    -0.932992798834739,
                                    -0.923879532511287,
                                    -0.914209755703531,
                                    -0.903989293123443,
                                    -0.893224301195515,
                                    -0.881921264348355,
                                    -0.870086991108711,
                                    -0.857728610000272,
                                    -0.844853565249707,
                                    -0.831469612302545,
                                    -0.817584813151584,
                                    -0.803207531480645,
                                    -0.788346427626606,
                                    -0.773010453362737,
                                    -0.757208846506485,
                                    -0.740951125354959,
                                    -0.724247082951467,
                                    -0.707106781186548,
                                    -0.689540544737067,
                                    -0.671558954847019,
                                    -0.653172842953777,
                                    -0.634393284163646,
                                    -0.615231590580627,
                                    -0.595699304492433,
                                    -0.575808191417845,
                                    -0.555570233019602,
                                    -0.534997619887097,
                                    -0.514102744193222,
                                    -0.492898192229784,
                                    -0.471396736825998,
                                    -0.449611329654607,
                                    -0.427555093430283,
                                    -0.405241314004990,
                                    -0.382683432365090,
                                    -0.359895036534988,
                                    -0.336889853392220,
                                    -0.313681740398892,
                                    -0.290284677254462,
                                    -0.266712757474899,
                                    -0.242980179903264,
                                    -0.219101240156870,
                                    -0.195090322016129,
                                    -0.170961888760302,
                                    -0.146730474455362,
                                    -0.122410675199216,
                                    -0.098017140329561,
                                    -0.073564563599667,
                                    -0.049067674327418,
                                    -0.024541228522912

};

/** 255 samples of one period of a square wave. */
static const double squareTable[] = {
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
};

/** 255 samples of one period of a saw wave.*/
static const double sawTable[] = { 0,
                                   0.007812500000000,
                                   0.015625000000000,
                                   0.023437500000000,
                                   0.031250000000000,
                                   0.039062500000000,
                                   0.046875000000000,
                                   0.054687500000000,
                                   0.062500000000000,
                                   0.070312500000000,
                                   0.078125000000000,
                                   0.085937500000000,
                                   0.093750000000000,
                                   0.101562500000000,
                                   0.109375000000000,
                                   0.117187500000000,
                                   0.125000000000000,
                                   0.132812500000000,
                                   0.140625000000000,
                                   0.148437500000000,
                                   0.156250000000000,
                                   0.164062500000000,
                                   0.171875000000000,
                                   0.179687500000000,
                                   0.187500000000000,
                                   0.195312500000000,
                                   0.203125000000000,
                                   0.210937500000000,
                                   0.218750000000000,
                                   0.226562500000000,
                                   0.234375000000000,
                                   0.242187500000000,
                                   0.250000000000000,
                                   0.257812500000000,
                                   0.265625000000000,
                                   0.273437500000000,
                                   0.281250000000000,
                                   0.289062500000000,
                                   0.296875000000000,
                                   0.304687500000000,
                                   0.312500000000000,
                                   0.320312500000000,
                                   0.328125000000000,
                                   0.335937500000000,
                                   0.343750000000000,
                                   0.351562500000000,
                                   0.359375000000000,
                                   0.367187500000000,
                                   0.375000000000000,
                                   0.382812500000000,
                                   0.390625000000000,
                                   0.398437500000000,
                                   0.406250000000000,
                                   0.414062500000000,
                                   0.421875000000000,
                                   0.429687500000000,
                                   0.437500000000000,
                                   0.445312500000000,
                                   0.453125000000000,
                                   0.460937500000000,
                                   0.468750000000000,
                                   0.476562500000000,
                                   0.484375000000000,
                                   0.492187500000000,
                                   0.500000000000000,
                                   0.507812500000000,
                                   0.515625000000000,
                                   0.523437500000000,
                                   0.531250000000000,
                                   0.539062500000000,
                                   0.546875000000000,
                                   0.554687500000000,
                                   0.562500000000000,
                                   0.570312500000000,
                                   0.578125000000000,
                                   0.585937500000000,
                                   0.593750000000000,
                                   0.601562500000000,
                                   0.609375000000000,
                                   0.617187500000000,
                                   0.625000000000000,
                                   0.632812500000000,
                                   0.640625000000000,
                                   0.648437500000000,
                                   0.656250000000000,
                                   0.664062500000000,
                                   0.671875000000000,
                                   0.679687500000000,
                                   0.687500000000000,
                                   0.695312500000000,
                                   0.703125000000000,
                                   0.710937500000000,
                                   0.718750000000000,
                                   0.726562500000000,
                                   0.734375000000000,
                                   0.742187500000000,
                                   0.750000000000000,
                                   0.757812500000000,
                                   0.765625000000000,
                                   0.773437500000000,
                                   0.781250000000000,
                                   0.789062500000000,
                                   0.796875000000000,
                                   0.804687500000000,
                                   0.812500000000000,
                                   0.820312500000000,
                                   0.828125000000000,
                                   0.835937500000000,
                                   0.843750000000000,
                                   0.851562500000000,
                                   0.859375000000000,
                                   0.867187500000000,
                                   0.875000000000000,
                                   0.882812500000000,
                                   0.890625000000000,
                                   0.898437500000000,
                                   0.906250000000000,
                                   0.914062500000000,
                                   0.921875000000000,
                                   0.929687500000000,
                                   0.937500000000000,
                                   0.945312500000000,
                                   0.953125000000000,
                                   0.960937500000000,
                                   0.968750000000000,
                                   0.976562500000000,
                                   0.984375000000000,
                                   0.992187500000000,
                                   -0.992187500000000,
                                   -0.992187500000000,
                                   -0.984375000000000,
                                   -0.976562500000000,
                                   -0.968750000000000,
                                   -0.960937500000000,
                                   -0.953125000000000,
                                   -0.945312500000000,
                                   -0.937500000000000,
                                   -0.929687500000000,
                                   -0.921875000000000,
                                   -0.914062500000000,
                                   -0.906250000000000,
                                   -0.898437500000000,
                                   -0.890625000000000,
                                   -0.882812500000000,
                                   -0.875000000000000,
                                   -0.867187500000000,
                                   -0.859375000000000,
                                   -0.851562500000000,
                                   -0.843750000000000,
                                   -0.835937500000000,
                                   -0.828125000000000,
                                   -0.820312500000000,
                                   -0.812500000000000,
                                   -0.804687500000000,
                                   -0.796875000000000,
                                   -0.789062500000000,
                                   -0.781250000000000,
                                   -0.773437500000000,
                                   -0.765625000000000,
                                   -0.757812500000000,
                                   -0.750000000000000,
                                   -0.742187500000000,
                                   -0.734375000000000,
                                   -0.726562500000000,
                                   -0.718750000000000,
                                   -0.710937500000000,
                                   -0.703125000000000,
                                   -0.695312500000000,
                                   -0.687500000000000,
                                   -0.679687500000000,
                                   -0.671875000000000,
                                   -0.664062500000000,
                                   -0.656250000000000,
                                   -0.648437500000000,
                                   -0.640625000000000,
                                   -0.632812500000000,
                                   -0.625000000000000,
                                   -0.617187500000000,
                                   -0.609375000000000,
                                   -0.601562500000000,
                                   -0.593750000000000,
                                   -0.585937500000000,
                                   -0.578125000000000,
                                   -0.570312500000000,
                                   -0.562500000000000,
                                   -0.554687500000000,
                                   -0.546875000000000,
                                   -0.539062500000000,
                                   -0.531250000000000,
                                   -0.523437500000000,
                                   -0.515625000000000,
                                   -0.507812500000000,
                                   -0.500000000000000,
                                   -0.492187500000000,
                                   -0.484375000000000,
                                   -0.476562500000000,
                                   -0.468750000000000,
                                   -0.460937500000000,
                                   -0.453125000000000,
                                   -0.445312500000000,
                                   -0.437500000000000,
                                   -0.429687500000000,
                                   -0.421875000000000,
                                   -0.414062500000000,
                                   -0.406250000000000,
                                   -0.398437500000000,
                                   -0.390625000000000,
                                   -0.382812500000000,
                                   -0.375000000000000,
                                   -0.367187500000000,
                                   -0.359375000000000,
                                   -0.351562500000000,
                                   -0.343750000000000,
                                   -0.335937500000000,
                                   -0.328125000000000,
                                   -0.320312500000000,
                                   -0.312500000000000,
                                   -0.304687500000000,
                                   -0.296875000000000,
                                   -0.289062500000000,
                                   -0.281250000000000,
                                   -0.273437500000000,
                                   -0.265625000000000,
                                   -0.257812500000000,
                                   -0.250000000000000,
                                   -0.242187500000000,
                                   -0.234375000000000,
                                   -0.226562500000000,
                                   -0.218750000000000,
                                   -0.210937500000000,
                                   -0.203125000000000,
                                   -0.195312500000000,
                                   -0.187500000000000,
                                   -0.179687500000000,
                                   -0.171875000000000,
                                   -0.164062500000000,
                                   -0.156250000000000,
                                   -0.148437500000000,
                                   -0.140625000000000,
                                   -0.132812500000000,
                                   -0.125000000000000,
                                   -0.117187500000000,
                                   -0.109375000000000,
                                   -0.101562500000000,
                                   -0.093750000000000,
                                   -0.085937500000000,
                                   -0.078125000000000,
                                   -0.070312500000000,
                                   -0.062500000000000,
                                   -0.054687500000000,
                                   -0.046875000000000,
                                   -0.039062500000000,
                                   -0.031250000000000,
                                   -0.023437500000000,
                                   -0.015625000000000,
                                   -0.007812500000000 };

double
WaveTable_sample(WaveType type, double angle)
{
    double idxExact = (angle)*WT_N_SAMPLES;
    double idxLow;
    double idxFrac = modf(idxExact, &idxLow);
    int idx = idxLow;
    int nextIdx = (idx < (WT_N_SAMPLES - 1)) ? idx + 1 : 0;

    const double* table;
    switch (type) {
        case WAVETYPE_SINE: {
            table = sineTable;
            break;
        }
        case WAVETYPE_SQUARE: {
            table = squareTable;
            break;
        }
        case WAVETYPE_SAW: {
            table = sawTable;
            break;
        }
        default:
            return -1;
            break;
    }
    return ((idxFrac * table[idx]) + ((1.0 - idxFrac) * table[nextIdx]));
}
