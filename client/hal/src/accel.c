/**
 * @file accel.c
 * @brief Implementation of the Accelerometer module.
 * @author Spencer Leslie 301571329
 */
#include "hal/accel.h"
#include "hal/i2c.h"
#include <stdio.h>

/** Handle to the i2c bus for communicating with the accelerometer. */
static I2C_BusHandle i2c;

/* Upon calling Accel_setCape(), ZEN_CAPE is set to 'r' for red, and 'g' for
 * green. The process to read the accel on each cape differs by a non-trivial
 * amount. Therefore, in most accel functions, we check the ZEN_CAPE with an if
 * statement, then handle both cases completely separately, as opposed to trying
 * to over-generalize the fn.
 */
static char ZEN_CAPE = '0';

#define ACCEL_STR_RED "18"
#define ACCEL_STR_GREEN "1c"

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

/**
 * Sets the internal ZEN_CAPE variable to 'r' for red, or 'g' for
 * green. Essentially the same as fileio_pipe_command, but specifically runs the
 * command "i2cdetect -y -r 1", which displays I2C device addresses. While
 * reading through the pipe output, we check for the specific accel addresses.
 */
static int
Accel_setCape(void)
{
    // It's probably possible to do this in a more generalized sense by
    // altering the original fileio_pipe_command, but this works for now.

    // We could probably also integrate this into our config file.

    FILE* pipe = popen("i2cdetect -y -r 1", "r");
    if (!pipe) {
        return -1;
    }
    char buffer[FILEIO_MED_BUFSIZE];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL) {
            break;
        }
        // Check buffer for the substring matching the accel regAddr.
        // If found, then set.
        if (strstr(buffer, ACCEL_STR_RED)) {
            ZEN_CAPE = 'r';
        } else if (strstr(buffer, ACCEL_STR_GREEN)) {
            ZEN_CAPE = 'g';
        }
    }

    // Explicit error handling
    if (ZEN_CAPE != 'r' && ZEN_CAPE != 'g') {
        ZEN_CAPE = '0';
    }

    // Get the exit code from the pipe; non-zero is an error:
    return WEXITSTATUS(pclose(pipe));
}

int
Accel_open(void)
{
    uint8_t addr;
    uint8_t cfg;
    uint8_t CTRL_REG1;

    if (I2C_openBus(I2C_BUS1, &i2c) < 0) {
        perror("i2c open");
        return -1;
    }

    // The second error check should guarantee that ZEN_CAPE is either 'r' or
    // 'g' past this point.
    if (Accel_setCape() < 0 || ZEN_CAPE == '0') {
        perror("accel setCape");
        return -1;
    }

    if (ZEN_CAPE == 'r') {
        puts("----------------------");
        puts("You have a Red cape.");
        puts("----------------------");

        addr = 0x18;
        cfg = (1 << 5) | 0x7;
        CTRL_REG1 = 0x20;

    } else if (ZEN_CAPE == 'g') {

        puts("----------------------");
        puts("You have a Green cape.");
        puts("----------------------");

        addr = 0x1C;
        cfg = 0x01;
        CTRL_REG1 = 0x2A;
    }

    if (I2C_selectDevice(&i2c, addr) < 0) {
        perror("i2c select device");
        return -1;
    }

    if (I2C_write(&i2c, CTRL_REG1, &cfg, 1) < 0) {
        perror("config");
        I2C_closeBus(&i2c);
        return -1;
    }

    return 0;
}

void
Accel_close(void)
{
    uint8_t CTRL_REG1;

    if (ZEN_CAPE == 'r') {
        CTRL_REG1 = 0x20;
    } else if (ZEN_CAPE == 'g') {
        CTRL_REG1 = 0x2A;
    }

    uint8_t z = 0;
    I2C_write(&i2c, CTRL_REG1, &z, 1);
    I2C_closeBus(&i2c);
}

int
Accel_read(Accel_Sample* out)
{
    // Reading 6 bytes of data, but we need + 1 because on Zen Cape green, the
    // first byte is always 0xFF.
    uint8_t buf[7];

    // This variable is not terribly important to understand; it just happens
    // to fall into a pattern that represents both capes accurately.
    int indexAdjust;

    if (ZEN_CAPE == 'r') {
        if (I2C_read(&i2c, 0x28 + 0x80, buf, 6) < 0) {
            return -1;
        }
        indexAdjust = 0;
    } else if (ZEN_CAPE == 'g') {
        if (I2C_read(&i2c, 0x01, buf, 7) < 0) {
            return -1;
        }
        indexAdjust = 2;
    }

    // Parsing the register data is the same across both capes.

    /* Samples are given as 12-bit 2's complement numbers
     * left-aligned in a 16-bit field given as 2 bytes.
     *
     * We first put the two bytes together and right-align them */
    out->x = (((buf[1] << 8) | buf[0 + indexAdjust]) >> 4);
    /* Then subtract 2^11 if the high bit is set. */
    out->x -= ((1 << 11) & out->x) << 1;

    /* And repeat for the other axes. */
    out->y = ((buf[3] << 8) | buf[2 + indexAdjust]) >> 4;
    out->y -= ((1 << 11) & out->y) << 1;

    out->z = ((buf[5] << 8) | buf[4 + indexAdjust]) >> 4;
    out->z -= ((1 << 11) & out->z) << 1;
    /* The Z register will always feel ~1G from normal gravity when it is on
     * the table. We make a crude correction for it. */
    // out->z -= 1024;

    return 0;
}
