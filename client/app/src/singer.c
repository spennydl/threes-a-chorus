#include "singer.h"
#include "com/timeutils.h"
#include "sensory.h"
#include "shutdown.h"
#include <stdio.h>

#define MAX_REPORT_SIZE 1024

static int run = 1;
static char report[MAX_REPORT_SIZE];

/** Shutdown handler. */
static void
_shutdown_handler(int status);

/** Prints the current sensory state. */
static void
_printSensoryReport(Sensory_State* state);

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

        _printSensoryReport(state);

        long long elapsed = Timeutils_getTimeInMs() - start;
        long long toSleep = MS_TO_WAIT_BETWEEN_UPDATES - elapsed;
        Timeutils_sleepForMs(toSleep);
    }

    _singerShutdown();

    return 0;
}
