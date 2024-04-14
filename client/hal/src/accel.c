/**
 * @file accel.c
 * @brief Implementation of the Accelerometer module.
 * @author Spencer Leslie 301571329
 */
#include "hal/accel.h"
#include "com/config.h"
#include "hal/i2c.h"
#include <stdio.h>

/** Handle to the i2c bus for communicating with the accelerometer. */
static I2C_BusHandle i2c;

//////////////////////////////////////////
// comment
#if (0)
// TODO: Integrator in its own module
/** Integrator for the 3 accelerometer axes. */
typedef struct
{
    uint8_t x;
    uint8_t x_trigger;
    uint8_t y;
    uint8_t y_trigger;
    uint8_t z;
    uint8_t z_trigger;
} accel_integrator_t;
static accel_integrator_t integrator = { 0 };

/** Reads from the accelerometer and updates the integrator. */
static void
_accel_update(void);

static void
_accel_update(void)
{
    accel_sample_t sample;
    accel_read(&sample);

#define SAMPLE_AXIS(AX)                                                        \
    do {                                                                       \
        sample.AX = sample.AX > 0 ? sample.AX : sample.AX * -1;                \
        if (sample.AX > TRIGGER_THRESHOLD) {                                   \
            if (integrator.AX < SAMPLES_TO_TRIGGER) {                          \
                integrator.AX += 1;                                            \
            }                                                                  \
        } else if (sample.AX < HYSTERISIS_THRESHOLD) {                         \
            if (integrator.AX > 0) {                                           \
                integrator.AX -= 1;                                            \
            }                                                                  \
        }                                                                      \
    } while (0)

    SAMPLE_AXIS(x);
    SAMPLE_AXIS(y);
    SAMPLE_AXIS(z);

#undef SAMPLE_AXIS
}

int
accel_get_events(accel_event_t* events)
{
    /*
     * This debounce strategy used was inspired by Kenneth Kuhn,
     * their implementation can be found at
     * http://www.kennethkuhn.com/electronics/debounce.c
     */
    _accel_update();

    int n_events = 0;

#define xI 0
#define yI 1
#define zI 2
#define CHECK_AXIS_EVENTS(AX)                                                  \
    do {                                                                       \
        events[AX##I] = ACCEL_EVENT_NONE;                                      \
        if (integrator.AX == SAMPLES_TO_TRIGGER && !integrator.AX##_trigger) { \
            integrator.AX##_trigger = 1;                                       \
            events[AX##I] = ACCEL_EVENT_JOLT;                                  \
            n_events++;                                                        \
        } else if (integrator.AX == 0 && integrator.AX##_trigger) {            \
            integrator.AX##_trigger = 0;                                       \
        }                                                                      \
    } while (0)

    CHECK_AXIS_EVENTS(x);
    CHECK_AXIS_EVENTS(y);
    CHECK_AXIS_EVENTS(z);

#undef xI
#undef yI
#undef zI
#undef CHECK_AXIS_EVENTS

    return n_events;
}
#endif
// end comment
//////////////////////////////////////////

int
Accel_open(void)
{
    if (I2C_openBus(I2C_BUS1, &i2c) < 0) {
        perror("i2c open");
        return -1;
    }

    if (I2C_selectDevice(&i2c, ACCEL_ADDR) < 0) {
        perror("i2c select device");
        return -1;
    }

    uint8_t cfg = ACCEL_ACTIVE_MODE;
    if (I2C_write(&i2c, ACCEL_CTRL_REG1, &cfg, 1) < 0) {
        perror("config");
        I2C_closeBus(&i2c);
        return -1;
    }

    return 0;
}

void
Accel_close(void)
{
    uint8_t z = 0;
    I2C_write(&i2c, ACCEL_CTRL_REG1, &z, 1);
    I2C_closeBus(&i2c);
}

int
Accel_read(Accel_Sample* out)
{
    // Reading 6 bytes of data, but we need + 1 because on Zen Cape green, the
    // first byte is always 0xFF.
    uint8_t buf[ACCEL_NUM_BYTES_TO_READ];

    if (I2C_read(&i2c, ACCEL_DATA_READ, buf, ACCEL_NUM_BYTES_TO_READ) < 0) {
        return -1;
    }

    // INDEX_ADJUST is not terribly important to understand; it just happens
    // to fall into a pattern that represents both capes accurately.

    /* Samples are given as 12-bit 2's complement numbers
     * left-aligned in a 16-bit field given as 2 bytes.
     *
     * We first put the two bytes together and right-align them */
    out->x = (((buf[1] << 8) | buf[0 + ACCEL_INDEX_ADJUST]) >> 4);
    /* Then subtract 2^11 if the high bit is set. */
    out->x -= ((1 << 11) & out->x) << 1;

    /* And repeat for the other axes. */
    out->y = ((buf[3] << 8) | buf[2 + ACCEL_INDEX_ADJUST]) >> 4;
    out->y -= ((1 << 11) & out->y) << 1;

    out->z = ((buf[5] << 8) | buf[4 + ACCEL_INDEX_ADJUST]) >> 4;
    out->z -= ((1 << 11) & out->z) << 1;
    /* The Z register will always feel ~1G from normal gravity when it is on
     * the table. We make a crude correction for it. */
    // out->z -= 1024;

    return 0;
}
