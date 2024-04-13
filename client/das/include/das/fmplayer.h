/**
 * @file fmplayer.h
 * @brief Drives a synthesizer and sends its output to the audio driver for
 * playback.
 *
 * This module manages an FmSynthesizer internally and is meant to be used as
 * the primary interface to it.
 *
 * @author Spencer Leslie 301571329
 */

#pragma once

#include "das/fm.h"

/** Note control. Specifies turing a note on, off, or playing a stoccato note
 * which results from turning a note on and immediately gating it.*/
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

/**
 * Performs the note control operation to turn a note on, off, or play a
 * stoccato note.
 */
void
FmPlayer_controlNote(FmPlayer_NoteCtrl ctrl);

/**
 * Update the active synth parameters.
 *
 * If params is NULL then the last applied voice will be applied again. This is
 * useful if modulation params have been tweaked and you want to reset to the
 * last applied voice.
 *
 * @param params The parameters to update.
 */
void
FmPlayer_setSynthVoice(const FmSynthParams* params);

/**
 * Update the wave type played by the given operator.
 *
 * @param op The operator to update.
 * @param wave The new wave type.
 */
void
FmPlayer_updateOperatorWaveType(FmOperator op, WaveType wave);

/**
 * Updates the CM ratio of the current operator.
 *
 * @param op The operator to update.
 * @param cm The new cm ratio.
 */
void
FmPlayer_updateOperatorCm(FmOperator op, float cm);

/**
 * Updates an operator's output strength in the mix.
 *
 * @param op The operator to update.
 * @param outStrength The new output strength of the operator.
 */
void
FmPlayer_updateOperatorOutputStrength(FmOperator op, float outStrength);

/**
 * Fixes an operator to a note.
 *
 * @param op The operator to fix.
 * @param note The note to fix the operator to.
 */
void
FmPlayer_fixOperatorToNote(FmOperator op, Note note);

/**
 * Updates the current algorithmic connections for an operator.
 *
 * @param op The operator to update.
 * @param moddingOp The modulating operator in the connection.
 * @modIndex The new modulation index for the connection.
 */
void
FmPlayer_updateOperatorAlgorithmConnection(FmOperator op,
                                           FmOperator moddingOp,
                                           float modIndex);

/**
 * Sets the currently playing note.
 *
 * @param note The note to play.
 */
void
FmPlayer_setNote(Note note);

/**
 * @brief Close and tear down the player.
 */
void
FmPlayer_close(void);
