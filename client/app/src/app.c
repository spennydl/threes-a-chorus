#include <poll.h>
#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "das/fm.h"
#include "das/fmplayer.h"
#include "das/melodygen.h"
#include "das/sequencer.h"
#include "hal/rfid.h"
#include "netMidiPlayer.h"
#include "singer.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>

static bool isRunning = false;
static int currentTagId = 0xFF;

static Mood debugMood = { .emotion = EMOTION_HAPPY, .magnitude = 1.0 };

static const FmSynthParams* currentVoice = &FM_DEFAULT_PARAMS;

static bool
runMidiPlayer(int channel, char* ip)
{
    if (NetMidi_openMidiChannel(ip, channel) < 0) {
        printf("Error: Could not open midi channel\n");
        return false;
    }

    return true;
}

/** Timeout for the poll() on stdin. */
#define SHUTDOWN_STDINPOLL_TIMEOUT_MS 100
/** Buffer size for the buffer used to consume stdin. */
#define SHUTDOWN_STDINPOLL_BUFSIZ 128

void
App_runApp(char* serverIp)
{
    (void)debugMood;

    isRunning = true;
    bool midiPlayerIsRunning = false;
    bool onRfid = false;

    App_onSequencerLoop();

    while (isRunning) {

        currentTagId = Rfid_getCurrentTagId();
        onRfid = currentTagId != 0xFF;

        if (onRfid) {
            if (!midiPlayerIsRunning) {
                // If not running yet, start midi player
                Sequencer_stop();
                printf("Found tag. Id is %d -> %d\n",
                       currentTagId,
                       currentTagId % 16);
                midiPlayerIsRunning =
                  runMidiPlayer(currentTagId % 16, serverIp);
                SegDisplay_setIsSinging(true);
            }
            // otherwise let the midi player play
        } else {
            // If not on tag but player is running, shut down
            if (midiPlayerIsRunning) {
                NetMidi_stop();
                midiPlayerIsRunning = false;
                Sequencer_start();
                SegDisplay_setIsSinging(false);
            }
            Singer_update();
        }

        float pot = Sensory_getPotLevel();
        float light = Sensory_getLightReading();

        if (fabs(light) > 0.1) {
            FmPlayer_updateOperatorCm(FM_OPERATOR1, light * 10);
        } else {
            FmPlayer_updateOperatorCm(
              FM_OPERATOR1, currentVoice->opParams[FM_OPERATOR1].CmRatio);
        }

        if (fabs(pot) > 0.1) {
            FmPlayer_updateOperatorAlgorithmConnection(
              FM_OPERATOR0, FM_OPERATOR1, pot * 8800);
        } else {
            FmPlayer_updateOperatorAlgorithmConnection(
              FM_OPERATOR0,
              FM_OPERATOR1,
              currentVoice->opParams[FM_OPERATOR0]
                .algorithmConnections[FM_OPERATOR1]);
        }

        Timeutils_sleepForMs(SHUTDOWN_STDINPOLL_TIMEOUT_MS);
    }
}

void
App_onSequencerLoop()
{
    Sequencer_stop();
    Sequencer_clear();
    const Mood* mood = Singer_getMood();
    MelodyGenParams melodyParams;

    // Retrieve the params for an emotion. Each emotion is associated with both
    // a voice (the timbre) and a melody (the notes to be played).
    // Voice params defined in fm.h and config.h
    // Melody params defined in melodygen.h
    switch (mood->emotion) {
        case EMOTION_HAPPY:
            currentVoice = &VOICE_HAPPY;
            melodyParams = happyParams;
            break;
        case EMOTION_SAD:
            currentVoice = &VOICE_SAD;
            melodyParams = sadParams;
            break;
        case EMOTION_ANGRY:
            currentVoice = &VOICE_ANGRY;
            melodyParams = angryParams;
            break;
        case EMOTION_OVERSTIMULATED:
            currentVoice = &VOICE_OVERSTIMULATED;
            melodyParams = overstimulatedParams;
            break;
        case EMOTION_NEUTRAL:
            currentVoice = &VOICE_NEUTRAL;
            melodyParams = neutralParams;
            break;
        default:
            currentVoice = &VOICE_NEUTRAL;
            melodyParams = neutralParams;
            break;
    }

    // For melody params, factor in the mood magnitude [0.0 - 1.0]
    melodyParams.jumpChance *= mood->magnitude;
    melodyParams.noteDensity *= mood->magnitude;
    melodyParams.upDownTendency *= mood->magnitude;
    melodyParams.stoccatoLegatoTendency *= mood->magnitude;

    // Pass the final params to the respective functions
    FmPlayer_setSynthVoice(currentVoice);
    Melody_generateToSequencer(&melodyParams);
    Sequencer_reset();
}

void
App_shutdownApp()
{
    isRunning = false;
}

bool
App_isRunning()
{
    return isRunning;
}
