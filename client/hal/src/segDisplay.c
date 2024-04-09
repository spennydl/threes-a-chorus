#include "hal/segDisplay.h"

#define GPIO_DIR_LEFT "/sys/class/gpio/gpio61/value"
#define GPIO_DIR_RIGHT "/sys/class/gpio/gpio44/value"

#define SEGDISPLAY_SLEEP_ms 5

// Decrease to get more blinks, increase to get less blinks.
// From testing this module alone, 400 results in around ~5 sec between blinks.
#define BLINK_THRESHOLD 200

// Admin vars
static atomic_bool _isSinging = false;
static Mood* mood;

static I2C_BusHandle i2c;
static pthread_t _segDisplayThread;
static bool init = false;

//////////////////////// Function Prototypes /////////////////////////////
static void
SegDisplay_turnOffDigits(void);

static void
SegDisplay_displayLeftPattern(uint8_t top, uint8_t bot);

static void
SegDisplay_displayRightPattern(uint8_t top, uint8_t bot);

// Display a pattern on each digit by rapidly switching on and off between
// turning on the left and right digit.
static void
SegDisplay_displayPattern(const uint8_t* pattern);

// Threaded function that retrieves the current mood, and displays the
// corresponding emotion. Threaded separate from app.c because we don't want
// flicker to be caused by other app activities getting in the way of the
// displaying.
static void*
_display(void* args);

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

// Display a pattern on each digit by rapidly switching on and off between
// turning on the left and right digit.
static void
SegDisplay_displayPattern(const uint8_t* pattern)
{
    assert(init);

    SegDisplay_turnOffDigits();

    SegDisplay_displayLeftPattern(pattern[PATTERN_LEFT_TOP],
                                  pattern[PATTERN_LEFT_BOT]);
    Timeutils_sleepForMs(SEGDISPLAY_SLEEP_ms);

    SegDisplay_turnOffDigits();

    SegDisplay_displayRightPattern(pattern[PATTERN_RIGHT_TOP],
                                   pattern[PATTERN_RIGHT_BOT]);
    Timeutils_sleepForMs(SEGDISPLAY_SLEEP_ms);
}

static void*
_display(void* args)
{
    (void)args;

    while (true) {
        // Singing is a unique emotion, in that it is not derived from sensory
        // params. The singing emotion has display priority above all
        // other emotions if the BBG is currently on RFID (i.e. it's singing).
        if (_isSinging) {
            SegDisplay_displayEmotion(EMOTION_SINGING);
        } else {
            mood = Singer_getMood();
            SegDisplay_displayEmotion(mood->emotion);
        }
    }

    return NULL;
}

//////////////////// External Functions //////////////////////////

int
SegDisplay_init(void)
{
    // Used to randomize the duration of blinking and opening eyes
    srand(time(NULL));

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

    // Create the thread
    int rc;
    if ((rc = pthread_create(&_segDisplayThread, NULL, _display, NULL)) != 0) {
        perror("segDisplay pthread_create");
        exit(-1);
    }

    return 0;
}

void
SegDisplay_shutdown(void)
{
    assert(init);

    pthread_join(_segDisplayThread, NULL);
    I2C_closeBus(&i2c);
    SegDisplay_turnOffDigits();

    init = false;
}

void
SegDisplay_setIsSinging(bool isSinging)
{
    _isSinging = isSinging;
}

void
SegDisplay_displayEmotion(Emotion emotion)
{
    const uint8_t* eyesPattern;
    const uint8_t* blinkPattern;

    // Set the digit patterns according to the mood.
    switch (emotion) {
        case EMOTION_HAPPY:
            eyesPattern = EYES_HAPPY;
            blinkPattern = BLINK_HAPPY;
            break;
        case EMOTION_SAD:
            eyesPattern = EYES_SAD;
            blinkPattern = BLINK_SAD;
            break;
        case EMOTION_ANGRY:
            eyesPattern = EYES_ANGRY;
            blinkPattern = BLINK_ANGRY;
            break;
        case EMOTION_OVERSTIMULATED:
            eyesPattern = EYES_OVERSTIMULATED;
            blinkPattern = BLINK_OVERSTIMULATED;
            break;
        case EMOTION_NEUTRAL:
            eyesPattern = EYES_NEUTRAL;
            blinkPattern = BLINK_NEUTRAL;
            break;
        case EMOTION_SINGING:
            eyesPattern = EYES_SINGING;
            blinkPattern = BLINK_SINGING;
            break;
        default:
            break;
    }

    // Special case that doesn't fit well in the generalized approach, because
    // of how dynamic and specific this pattern is
    if (emotion == EMOTION_OVERSTIMULATED) {
        SegDisplay_displayPattern(EYES_OVERSTIMULATED);
        Timeutils_sleepForMs(100);
        SegDisplay_displayPattern(BLINK_OVERSTIMULATED);
        Timeutils_sleepForMs(100);
        return;
    }

    // - assume each displayEmotion call takes ~15ms (5ms left + 5ms right +
    // overhead).
    // - we want to blink every 2-10s, which is ~6s on average.
    // - 6s = 6000ms / 15ms = 400. we want to blink 1/400 displayEmotion calls.

    // - implementing this with random % based chance instead of a time call
    // because we don't want our eyes to be stuck displaying a mood in a timed
    // while loop; the mood can change on a dime, and this approach allows that.
    bool blinking = Utils_getRandomIntBtwn(0, BLINK_THRESHOLD);
    if (blinking == 0) {
        printf("Blink at: %lld\n", Timeutils_getTimeInMs());

        // Blinks should last longer than the standard ~15ms, should they happen
        // to occur. We randomize from 50 - 300ms.
        int blinkDuration = Utils_getRandomIntBtwn(50, 300);
        long long currTime = Timeutils_getTimeInMs();
        while (currTime + blinkDuration > Timeutils_getTimeInMs()) {
            SegDisplay_displayPattern(blinkPattern);
        }
    } else {
        SegDisplay_displayPattern(eyesPattern);
    }
}