/**
 * @file fmplayer.h
 * @brief Drives a synthesizer and sends its output to the audio driver for
 * playback.
 *
 * Keeps audio samples in a chunked circular buffer.
 *
 * TODO: The circular buffer approach is probably not exactly what we want as it
 * introduces a butt-ton of latency. Likely what we need is more of a
 * frontbuffer/backbuffer approach.
 *
 * @author Spencer Leslie
 */

#pragma once

#include "com/config.h"
#include "das/fm.h"

#include "singer.h"

typedef enum
{
    NOTE_CTRL_NONE = 0,
    NOTE_CTRL_NOTE_ON,
    NOTE_CTRL_NOTE_OFF,
    NOTE_CTRL_NOTE_STOCCATO,
} FmPlayer_NoteCtrl;

/**
 * @brief Initialize the player.
 *
 * Creates a synthesizer with the given params, spins up a synthesizer thread
 * and a playback thread.
 *
 * @param params The synth params.
 * @return 1 on success, negative on failure.
 */
int
FmPlayer_initialize(const FmSynthParams* params);

void
FmPlayer_controlNote(FmPlayer_NoteCtrl ctrl);

/**
 * @brief Update the active synth parameters.
 *
 * @param params The parameters to update.
 */
void
FmPlayer_setSynthVoice(const FmSynthParams* params);

void
FmPlayer_updateOperatorWaveType(FmOperator op, WaveType wave);

void
FmPlayer_updateOperatorCm(FmOperator op, float cm);

void
FmPlayer_updateOperatorOutputStrength(FmOperator op, float outStrength);

void
FmPlayer_fixOperatorToNote(FmOperator op, Note note);

void
FmPlayer_updateOperatorAlgorithmConnection(FmOperator op,
                                           FmOperator moddingOp,
                                           float modIndex);

void
FmPlayer_setNote(Note note);
/**
 * @brief Close and tear down the player.
 */
void
FmPlayer_close(void);
