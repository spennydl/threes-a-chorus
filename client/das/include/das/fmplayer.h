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

#include "das/fm.h"

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
 * @brief Send a noteOn to the synthesizer.
 */
void
FmPlayer_noteOn(void);

/**
 * @brief Send a noteOff to the synthesizer.
 */
void
FmPlayer_noteOff(void);

/**
 * @brief Update the active synth parameters.
 *
 * @param params The parameters to update.
 */
void
FmPlayer_updateSynthParams(const FmSynthParams* params);

void
FmPlayer_setNote(Note note);
/**
 * @brief Close and tear down the player.
 */
void
FmPlayer_close(void);
