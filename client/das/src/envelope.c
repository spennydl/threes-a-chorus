/**
 * @file envelope.c
 * @brief ADSR envelope implementation.
 *
 * @author Spencer Leslie 301571329
 */
#include "das/envelope.h"
#include "com/pwl.h"
#include <stdbool.h>

/** State bit indicating trigger. */
#define ENV_TRIGGER_BIT (0x1)
/** State bit indicating gate. */
#define ENV_GATE_BIT (0x2)

/** Is the envelope triggered? */
static bool
_isTriggered(const Env_Envelope* env);

/** Has the envelope been gated? */
static bool
_isGated(const Env_Envelope* env);

static bool
_isTriggered(const Env_Envelope* env)
{
    return (env->state & ENV_TRIGGER_BIT) == ENV_TRIGGER_BIT;
}

static bool
_isGated(const Env_Envelope* env)
{
    return (env->state & ENV_GATE_BIT) == ENV_GATE_BIT;
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
    if (_isTriggered(env)) {
        float x = env->current;
        float min = env->min;

        value = Pwl_sample(&env->fn, x);
        if (value < min) {
            value = min;
        } else {
            min = -1;
        }

        x += env->step;

        if (x >= env->gatePoint && !_isGated(env)) {
            if (env->repeatPoint >= 0) {
                x = env->repeatPoint;
            } else {
                x = env->gatePoint;
            }
        }

        if (x > 1) {
            env->state = 0;
            x = 0;
        }

        env->current = x;
        env->min = min;
    }
    return value;
}

void
Env_trigger(Env_Envelope* env)
{
    if (_isTriggered(env)) {
        env->min = Pwl_sample(&env->fn, env->current);
    }
    env->current = 0;
    env->state = ENV_TRIGGER_BIT;
}

void
Env_gate(Env_Envelope* env)
{
    env->state |= ENV_GATE_BIT;
}
