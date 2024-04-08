#include "singer.h"
#include "com/timeutils.h"
#include "sensory.h"
#include "shutdown.h"

#define MAX_REPORT_SIZE 1024

// If we want the NEUTRAL mood to occur more generously, we can increase the
// numerator to give the hyperbolae a greater area.
#define HYPERBOLA_I(x) (1.0 / x)
#define HYPERBOLA_II(x) (-1.0 / x)

static int run = 1;
static char report[MAX_REPORT_SIZE];

static Mood mood = {
    .emotion = EMOTION_NEUTRAL,
    .magnitude = 0.0,
};

/** Shutdown handler. */
static void
_shutdown_handler(int status);

/** Prints the current sensory state. */
static void
_printSensoryReport(Sensory_State* state);

/** Update the singer's mood. */
static void
_updateMood(Sensory_State* state);

static void
_shutdown_handler(int status)
{
    (void)status;
    run = 0;
}

static void
_printSensoryReport(Sensory_State* state)
{
    snprintf(report,
             MAX_REPORT_SIZE,
             "accel[%s] :: pot[%s] :: dist[%s] :: light[%s] :: button[%s] -->> "
             "Index[%6.2f %6.2f]\n",
             Sensory_inputLevelToStr(state->accelState),
             Sensory_inputLevelToStr(state->potInteractionState),
             Sensory_inputLevelToStr(state->proximityState),
             Sensory_inputLevelToStr(state->lightLevel),
             Sensory_inputLevelToStr(state->buttonState),
             state->sensoryIndex,
             state->sensoryTolerance);

    printf("%s", report);
}

static void
_updateMood(Sensory_State* state)
{
    // Mood has two facets:
    //      (1) the emotion,
    //      (2) the magnitude.

    // We use the state param's latter two fields, sensoryIndex (x-axis) and
    // sensoryTolerance (y-axis), to determine the mood facets. We label them
    // here as x and y for brevity, and to invoke the image of a graph.
    Sensory_SensoryIndex x = state->sensoryIndex;
    Sensory_SensoryIndex y = state->sensoryTolerance;

    // (1) The emotion is determined by the graph quadrant that the coords
    // lie in, with each quadrant representing a different emotion.

    // First, we determine the quadrant based on the sign of the coords.
    // (Quadrant numbering starts at top-right and goes anticlockwise.)
    Quadrant quadrant;
    if (x > 0 && y > 0) {
        quadrant = QUADRANT_I;
    } else if (x < 0 && y > 0) {
        quadrant = QUADRANT_II;
    } else if (x < 0 && y < 0) {
        quadrant = QUADRANT_III;
    } else if (x > 0 && y < 0) {
        quadrant = QUADRANT_IV;
    } else {
        quadrant = QUADRANT_NONE; // one of x or y is exactly equal to 0, i.e.
                                  // hugging the axis
    }

    // After we determine the quadrant, we need to know if the sensory input is
    // high enough to evoke an emotion, or if the singer should remain NEUTRAL.

    // Graphically, this means the point falls within the area formed by four
    // asymptotes surrounding the origin point. We represent this "star twinkle"
    // shape with 2 hyperbola graphs. The graph we use depends on the quadrant;
    // y = 1/x covers Quadrants I and III, y = -1/x covers Quadrants II and IV.
    bool withinHyperbola = false;

    switch (quadrant) {
        case QUADRANT_NONE:
            withinHyperbola = true;
            break;
        case QUADRANT_I:
            if (y <= HYPERBOLA_I(x)) { // point falls below the hyperbola line
                withinHyperbola = true;
            } else {
                mood.emotion = EMOTION_HAPPY;
            }
            break;
        case QUADRANT_II:
            if (y <= HYPERBOLA_II(x)) {
                withinHyperbola = true;
            } else {
                mood.emotion = EMOTION_SAD;
            }
            break;
        case QUADRANT_III:
            if (y >= HYPERBOLA_I(x)) {
                withinHyperbola = true;
            } else {
                mood.emotion = EMOTION_ANGRY;
            }
            break;
        case QUADRANT_IV:
            if (y >= HYPERBOLA_II(x)) {
                withinHyperbola = true;
            } else {
                mood.emotion = EMOTION_OVERSTIMULATED;
            }
            break;
        default:
            break;
    }

    if (withinHyperbola) {
        mood.emotion = EMOTION_NEUTRAL;
    }

    // With that, we have finished determining the emotion. Now, the magnitude.

    // (2) The mood magnitude is calculated as the distance from the origin.

    // Normalize y between [-500, 500] for magnitude calculation
    // x is already normalized

    if (y > 500) {
        y = 500;
    }
    else if (y < -500) {
        y = -500;
    }

    y /= 500;

    // The max value of this is sqrt(2) because sqrt(1 + 1) so we can
    // divide by that to normalize this too

    mood.magnitude = (sqrt(powf(x, 2) + powf(y, 2))) / sqrt(2);
}

static void
_singerShutdown(void)
{
    Sensory_close();
}

int
Singer_intialize(void)
{
    // TODO: This will come from configuration
    Sensory_Preferences prefs = { .cAccelLow = 10,
                                  .cAccelHigh = -4,
                                  .cPot = -4,
                                  .cDistance = -2,
                                  .cLight = 3,
                                  .cButton = -5 };
    if (Sensory_initialize(&prefs) < 0) {
        fprintf(stderr, "Failed to initialize sensory system\n");
        return -1;
    }

    if (shutdown_install(_shutdown_handler) < 0) {
        Sensory_close();
        fprintf(stderr, "Failed to start shutdown listener\n");
        return -1;
    }

    return 0;
}

int
Singer_run(void)
{
    Sensory_beginSensing();

    const long long MS_TO_WAIT_BETWEEN_UPDATES = 1000;

    Sensory_State sensorState;
    Sensory_State* state = &sensorState;
    while (run) {
        long long start = Timeutils_getTimeInMs();

        Sensory_reportSensoryState(state);

        _updateMood(state);

        _printSensoryReport(state);

        long long elapsed = Timeutils_getTimeInMs() - start;
        long long toSleep = MS_TO_WAIT_BETWEEN_UPDATES - elapsed;
        Timeutils_sleepForMs(toSleep);
    }

    _singerShutdown();

    return 0;
}

Mood*
Singer_getMood(void)
{
    return &mood;
}