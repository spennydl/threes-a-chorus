/**
 * @file envelope.c
 * @brief ADSR envelope implementation.
 *
 * I expect this to change at some point.
 *
 * @author Spencer Leslie
 */
#include "das/envelope.h"
#include "com/pwl.h"

#define ENV_TRIGGER_BIT (0x1)
#define ENV_GATE_BIT (0x2)

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
        value = Pwl_sample(&env->fn, x);

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
