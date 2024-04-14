/**
 * @file singer.c
 * @brief Implementation of the singer module.
 * @author Spencer Leslie and Louie Lu.
 */
#include "com/config.h"

#include "das/fm.h"
#include "das/fmplayer.h"
#include "das/melodygen.h"
#include "das/sequencer.h"
#include "sensory.h"
#include "singer.h"
#include <math.h>
#include <stdio.h>

/** Max length of the report written to stdout */
#define MAX_REPORT_SIZE 1024

/** If we want the IDLE mood to occur more generously, we can increase the
 * numerator to give the hyperbola a greater area.
 *
 * Define this in config.h if you wish to use it. Sets the width of the
 * hyperbola bounding idleness. */
#ifndef IDLE_HYPERBOLA_NUMERATOR
#define IDLE_HYPERBOLA_NUMERATOR 1.0
#endif

/** Override in config.h. Determines the number of times to play a slow melody
 * before allowing a mood switch. */
#ifndef EMOTION_SLOW_PLAY_THRESHOLD
#define EMOTION_SLOW_PLAY_THRESHOLD 0
#endif

/** Override in config.h. Determines the number of times to play a fast or
 * medium tempo melody before allowing a mood switch. */
#ifndef EMOTION_FAST_PLAY_THRESHOLD
#define EMOTION_FAST_PLAY_THRESHOLD 0
#endif

/** Hyperbola for determining the idleness region. */
#define HYPERBOLA_I(x) ((IDLE_HYPERBOLA_NUMERATOR) / (x))
#define HYPERBOLA_II(x) ((-IDLE_HYPERBOLA_NUMERATOR) / (x))

/** Buffer for the report printed to stdout. */
static char report[MAX_REPORT_SIZE];

/** Singer's current mood. */
static Mood mood = {
    .emotion = EMOTION_IDLE,
    .magnitude = 0.0,
};

/** The current FmSynth voice that the singer is using. */
static const FmSynthParams* currentVoice;

/** Pointer to the current melody generation parameters. */
static _Atomic(const MelodyGenParams*) melodyParams;

/** How many times we've played a full melody since the last mood switch. */
static _Atomic int timesEmotionPlayed = 0;

/** The threshold of times to play a melody before allowing a mood switch. */
static int emotionPlayThreshold = EMOTION_SLOW_PLAY_THRESHOLD;

/** Prints the current sensory state. */
static void
_printSensoryReport(Sensory_State* state);

/** Update the singer's mood. */
static void
_updateMood(Sensory_State* state);

/** Attempts to switch the mood to a new emotion. May not actually switch if the
 * play threshold has not been reached. */
static void
_trySwitchToEmotion(const Emotion newEmotion);

/** Updates the synth voice and melody generation params to the configured
 * params for the current emotion. */
static void
_updateEmotionParams(void);

/** Callback called by the sequencer when it loops around. */
static void
_onSequencerLoop(void);

static void
_printSensoryReport(Sensory_State* state)
{
    snprintf(
      report,
      MAX_REPORT_SIZE,
      "accel[%s] :: pot[%s] :: dist[%s] :: light[%s %f] :: button[%s] -->> "
      "Index[%6.2f %6.2f]",
      Sensory_inputLevelToStr(state->accelState),
      Sensory_inputLevelToStr(state->potInteractionState),
      Sensory_inputLevelToStr(state->proximityState),
      Sensory_inputLevelToStr(state->lightLevel),
      Sensory_getLightReading(),
      Sensory_inputLevelToStr(state->buttonState),
      state->sensoryIndex,
      state->sensoryTolerance);

    printf("%s", "\033[K\r");
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
    // high enough to evoke an emotion, or if the singer should remain IDLE.

    // Graphically, this means the point falls within the area formed by four
    // asymptotes surrounding the origin point. We represent this "star twinkle"
    // shape with 2 hyperbola graphs. The graph we use depends on the quadrant;
    // y = 1/x covers Quadrants I and III, y = -1/x covers Quadrants II and IV.
    bool withinHyperbola = false;
    Emotion newEmotion = mood.emotion;

    switch (quadrant) {
        case QUADRANT_NONE:
            withinHyperbola = true;
            break;
        case QUADRANT_I:
            if (y <= HYPERBOLA_I(x)) { // point falls below the hyperbola line
                withinHyperbola = true;
            } else {
                newEmotion = EMOTION_HAPPY;
            }
            break;
        case QUADRANT_II:
            if (y <= HYPERBOLA_II(x)) {
                withinHyperbola = true;
            } else {
                newEmotion = EMOTION_SAD;
            }
            break;
        case QUADRANT_III:
            if (y >= HYPERBOLA_I(x)) {
                withinHyperbola = true;
            } else {
                newEmotion = EMOTION_ANGRY;
            }
            break;
        case QUADRANT_IV:
            if (y >= HYPERBOLA_II(x)) {
                withinHyperbola = true;
            } else {
                newEmotion = EMOTION_OVERSTIMULATED;
            }
            break;
        default:
            break;
    }

    if (withinHyperbola) {
        newEmotion = EMOTION_IDLE;
    }

    // With that, we have finished determining the emotion. Now, the magnitude.

    // (2) The mood magnitude is calculated as the distance from the origin.

    // Normalize y between [-500, 500] for magnitude calculation
    // x is already normalized

    if (y > 500) {
        y = 500;
    } else if (y < -500) {
        y = -500;
    }

    y /= 500;

    // The max value of this is sqrt(2) because sqrt(1 + 1) so we can
    // divide by that to normalize this too

    mood.magnitude = (sqrt(powf(x, 2) + powf(y, 2))) / sqrt(2);

    _trySwitchToEmotion(newEmotion);
}

static void
_trySwitchToEmotion(const Emotion newEmotion)
{
    if (mood.emotion != newEmotion &&
        timesEmotionPlayed > emotionPlayThreshold) {
        mood.emotion = newEmotion;

        _updateEmotionParams();

        emotionPlayThreshold = melodyParams->tempo == TEMPO_SLOW
                                 ? EMOTION_SLOW_PLAY_THRESHOLD
                                 : EMOTION_FAST_PLAY_THRESHOLD;
        timesEmotionPlayed = 0;
        Sequencer_reset();
    }
}

static void
_updateEmotionParams(void)
{
    // Retrieve the params for an emotion. Each emotion is associated with
    // both a voice (the timbre) and a melody (the notes to be played).
    // Voice params defined in fm.h and config.h
    // Melody params defined in melodygen.h
    switch (mood.emotion) {
        case EMOTION_HAPPY:
            currentVoice = &VOICE_HAPPY;
            melodyParams = &happyParams;
            break;
        case EMOTION_SAD:
            currentVoice = &VOICE_SAD;
            melodyParams = &sadParams;
            break;
        case EMOTION_ANGRY:
            currentVoice = &VOICE_ANGRY;
            melodyParams = &angryParams;
            break;
        case EMOTION_OVERSTIMULATED:
            currentVoice = &VOICE_OVERSTIMULATED;
            melodyParams = &overstimulatedParams;
            break;
        case EMOTION_NEUTRAL:
            currentVoice = &VOICE_NEUTRAL;
            melodyParams = &neutralParams;
            break;
        default:
            currentVoice = &VOICE_NEUTRAL;
            melodyParams = &neutralParams;
            break;
    }
}

static void
_onSequencerLoop(void)
{
    Sequencer_clear();
    MelodyGenParams params = *melodyParams;

    // For melody params, factor in the mood magnitude [0.0 - 1.0]
    params.jumpChance *= mood.magnitude;
    // melodyParams.noteDensity *= mood.magnitude;
    params.upDownTendency *= mood.magnitude;
    params.stoccatoLegatoTendency *= mood.magnitude;

    // Pass the final params to the respective functions
    FmPlayer_setSynthVoice(currentVoice);
    Melody_generateToSequencer(&params);
    timesEmotionPlayed++;
}

void
Singer_shutdown(void)
{
    Sequencer_destroy();

    Sensory_close();

    FmPlayer_close();
}

void
Singer_sing(void)
{
    Sequencer_reset();
}

void
Singer_rest(void)
{
    Sequencer_stop();
}

int
Singer_initialize(void)
{
    _updateEmotionParams();

    if (FmPlayer_initialize(currentVoice) < 0) {
        fprintf(stderr, "Failed to intiialize FMplayer\n");
        return -1;
    }

    if (Sensory_initialize(&sensoryPreferences) < 0) {
        fprintf(stderr, "Failed to initialize sensory system\n");
        FmPlayer_close();
        return -1;
    }

    if (Sequencer_initialize(120, _onSequencerLoop) < 0) {
        fprintf(stderr, "Failed to init sequencer\n");
        FmPlayer_close();
        Sensory_close();
        return -1;
    }

    Sensory_beginSensing();

    return 0;
}

void
Singer_modulateVoice(void)
{
    float pot = Sensory_getPotLevel();
    float light = Sensory_getLightReading();
    static bool potIsHigh = false;
    static bool lightIsHigh = false;

    if (fabs(light) > 0.1) {
        lightIsHigh = true;
        FmPlayer_updateOperatorCm(FM_OPERATOR1, light * 10);
    } else {
        if (lightIsHigh) {
            FmPlayer_setSynthVoice(NULL);
            lightIsHigh = false;
        }
    }

    if (fabs(pot) > 0.05) {
        FmPlayer_updateOperatorAlgorithmConnection(
          FM_OPERATOR0, FM_OPERATOR1, pot * 8800);
        potIsHigh = true;
    } else {
        if (potIsHigh) {
            FmPlayer_setSynthVoice(NULL);
            potIsHigh = false;
        }
    }
}

int
Singer_update(void)
{
    Sensory_State state;

    Sensory_reportSensoryState(&state);
    _updateMood(&state);

    _printSensoryReport(&state);

    return 0;
}

Mood*
Singer_getMood(void)
{
    return &mood;
}
