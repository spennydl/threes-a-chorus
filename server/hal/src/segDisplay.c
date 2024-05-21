#include "hal/segDisplay.h"

#define GPIO_DIR_LEFT "/sys/class/gpio/gpio61/value"
#define GPIO_DIR_RIGHT "/sys/class/gpio/gpio44/value"

// Error number patterns (00 and 33 respectively)
static const uint8_t okPattern[4] = { 0xA1, 0xD0, 0xA1, 0xD0 };
static const uint8_t errorPattern[4] = { 0x14, 0x05, 0x14, 0x05 };

// Admin vars
static I2C_BusHandle i2c;
static pthread_t _segDisplayThread;
static bool init = false;
static bool run = true;

//////////////////////// Function Prototypes /////////////////////////////
// Turn off any patterns on the display.
static void
SegDisplay_turnOffDigits(void);

// Display a pattern on the left digit given a top and bot pattern.
static void
SegDisplay_displayLeftPattern(uint8_t top, uint8_t bot);

// Display a pattern on the right digit given a top and bot pattern.
static void
SegDisplay_displayRightPattern(uint8_t top, uint8_t bot);

// Display a pattern once.
static void
SegDisplay_displayPattern(const uint8_t* pattern);

///////////////////// Prototype implementations ///////////////////////////

static void
SegDisplay_turnOffDigits(void)
{
    assert(init);

    fileio_write(GPIO_DIR_LEFT, "0");
    fileio_write(GPIO_DIR_RIGHT, "0");
}

static void
SegDisplay_displayLeftPattern(uint8_t top, uint8_t bot)
{
    assert(init);

    // Prepare the digit pattern
    I2C_write(&i2c, SEGDISPLAY_OUTB_REG, &top, 1);
    I2C_write(&i2c, SEGDISPLAY_OUTA_REG, &bot, 1);

    // Turn on the GPIO pin corresponding to the left LED digit
    fileio_write(GPIO_DIR_LEFT, "1");
}

static void
SegDisplay_displayRightPattern(uint8_t top, uint8_t bot)
{
    assert(init);

    // Prepare the digit pattern
    I2C_write(&i2c, SEGDISPLAY_OUTB_REG, &top, 1);
    I2C_write(&i2c, SEGDISPLAY_OUTA_REG, &bot, 1);

    // Turn on the GPIO pin corresponding to the right LED digit
    fileio_write(GPIO_DIR_RIGHT, "1");
}

static void
SegDisplay_displayPattern(const uint8_t* pattern)
{
    assert(init);

    SegDisplay_turnOffDigits();

    SegDisplay_displayLeftPattern(pattern[PATTERN_LEFT_TOP],
                                  pattern[PATTERN_LEFT_BOT]);

    SegDisplay_displayRightPattern(pattern[PATTERN_RIGHT_TOP],
                                   pattern[PATTERN_RIGHT_BOT]);
}

//////////////////// External Functions //////////////////////////

int
SegDisplay_init(void)
{
    // Open I2C BUS 1
    if (I2C_openBus(I2C_BUS1, &i2c) < 0) {
        perror("segDisplay open");
        return -1;
    }

    // Select segDisplay
    if (I2C_selectDevice(&i2c, SEGDISPLAY_I2C_ADDRESS) < 0) {
        perror("segDisplay select device");
        return -1;
    }

    // Set direction of both 8-bit ports on I2C GPIO extender to be outputs
    uint8_t cfg = 0x00;
    if (I2C_write(&i2c, SEGDISPLAY_DIRA_REG, &cfg, 1) < 0) {
        perror("segDisplay config");
        I2C_closeBus(&i2c);
        return -1;
    }
    if (I2C_write(&i2c, SEGDISPLAY_DIRB_REG, &cfg, 1) < 0) {
        perror("segDisplay config");
        I2C_closeBus(&i2c);
        return -1;
    }

    // Set direction of both GPIO pins to be outputs
    fileio_write("/sys/class/gpio/gpio61/direction", "out");
    fileio_write("/sys/class/gpio/gpio44/direction", "out");

    init = true;

    return 0;
}

void
SegDisplay_shutdown(void)
{
    assert(init);

    run = false;
    pthread_join(_segDisplayThread, NULL);
    I2C_closeBus(&i2c);
    SegDisplay_turnOffDigits();

    init = false;
}

void
SegDisplay_displayStatus(Server_Status status)
{
    if (status == SERVER_OK) {
        SegDisplay_displayPattern(okPattern);
    } else if (status == SERVER_ERROR) {
        SegDisplay_displayPattern(errorPattern);
    }
}
