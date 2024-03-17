/**
 * @file envelope.c
 * @brief ADSR envelope implementation.
 *
 * I expect this to change at some point.
 *
 * @author Spencer Leslie
 */
#include "das/envelope.h"

#define ENV_TRIGGER_BIT (0x1)
#define ENV_GATE_BIT (0x2)

float
Env_pwlSample(Env_PiecewiseLinearFn* pwl, float samplePoint)
{
    int i;
    float x0 = 0;
    float x1 = 0;
    float y0 = 0;
    float y1 = 0;

    // fprintf(stderr, "we have %u pts\n", pwl->pts);
    for (i = 0; i < pwl->pts - 1; i++) {
        // printf("yesp\n");
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

void
Env_prepareEnvelope(Env_Envelope* env, size_t sampleRate)
{
    float samplesPerMs = sampleRate * 0.001;
    float samplesPerEnv = env->lengthMs * samplesPerMs;
    env->step = 1.0 / samplesPerEnv;
    env->current = 0;
    env->state = 0;
}

float
Env_getValueAndAdvance(Env_Envelope* env)
{
    float value = 0;
    if (env->state & ENV_TRIGGER_BIT) {
        float x = env->current;
        value = Env_pwlSample(&env->fn, x);

        x += env->step;

        if (x >= env->gatePoint) {
            if (env->repeatPoint >= 0) {
                x = env->repeatPoint;
            } else if (!(env->state & ENV_GATE_BIT)) {
                x = env->gatePoint;
            }
        }

        if (x > 1) {
            env->state = 0;
            x = 0;
        }

        env->current = x;
    }
    return value;
}

void
Env_trigger(Env_Envelope* env)
{
    env->current = 0;
    env->state = ENV_TRIGGER_BIT;
}

void
Env_gate(Env_Envelope* env)
{
    env->state |= ENV_GATE_BIT;
}
