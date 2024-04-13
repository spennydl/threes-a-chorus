/**
 * @file sequencer.h
 * @brief A software sequencer.
 *
 * A sequencer is a sequence of slots containing notes that are played in order
 * and then repeated. Slots can be filled to create a melody that will be played
 * at a configurable tempo and looped.
 *
 * @author Spencer Leslie 301571329
 */
#pragma once

#include "fm.h"
#include "fmplayer.h"

/** Note divisions. */
#define SEQ_EIGHTH_NOTE_IN_QUARTER_NOTE 2
#define SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE 4
// TODO: thirtysecond note might be overkill
#define SEQ_THIRTYSECOND_NOTE 8

/** How many full beats the sequencer has. 8 beats is 2 bars. */
#define SEQ_BEAT_SLOTS 8

/** Sequencer status codes. */
#define SEQ_OK 0
#define SEQ_EALLOC -1
#define SEQ_EINVAL -2

/** Sequencer operations. Each slot contains one operation. */
typedef struct
{
    FmPlayer_NoteCtrl op;
    Note note;
    FmSynthParams* synthParams;
} SequencerOp;

/** Type of the index of the currently playing slot. */
typedef size_t SequencerIdx;
/** Type of a BPM. */
typedef size_t SequencerBpm;
/** Type of a BPM when given as a delta. */
typedef int SequencerBpmDelta;

/**
 * Type of an optional callback function that will be called immediately before
 * each time the sequencer plays its first slot.
 *
 * This function will be called from the sequencer thread. It's to be used to
 * set the melody before the sequencer starts playing.
 */
typedef void (*loopCallbackFn)(void);

/**
 * Initialize the sequencer at the given tempo.
 *
 * @param bpm The sequencer tempo.
 * @param callback An optional loop callback.
 * @return A status code. SEQ_OK on success, or < 0 on error.
 */
int
Sequencer_initialize(SequencerBpm bpm, loopCallbackFn callback);

/**
 * Get the index of a slot corresponding to a quarter, eighth, and sixteenth
 * note index.
 *
 * Quarter note indices are 0-indexed whole beats from 0 to SEQ_BEAT_SLOTS - 1.
 *
 * Eigth note indices are either 0 or 1 and correspond do the first or second
 * eigth note divisions of the whole beat.
 *
 * Sixteenth notes indices are either 0 or 1 and correspond to the sixteenth
 * note division following the specified eigth note.
 *
 * For example, beat 1 of the second bar has the following quarter, eigth, and
 * sixteenth indices:
 *
 * | Beat | Q  | E  | S |
 * |------|----|----|---|
 * | 2    | 1  | 0  | 0 |
 * | e    | 1  | 0  | 1 |
 * | and  | 1  | 1  | 0 |
 * | a    | 1  | 1  | 1 |
 *
 * @param quarter Quarter note index.
 * @param eigth Eighth note index.
 * @param sixteenth Sixteenth note index.
 * @return sequencerIdx A sequencer slot index.
 */
SequencerIdx
Sequencer_getSlotIndex(int quarter, int eighth, int sixteenth);

/**
 * Fill a sequencer slot with a not operation.
 *
 * @param idx The index of the slot to fill.
 * @param control Note control operation for the slot.
 * @param note The note to play for the slot.
 * @param synthParams Optional voice to change to when the slot is played.
 */
void
Sequencer_fillSlot(SequencerIdx idx,
                   FmPlayer_NoteCtrl control,
                   Note note,
                   FmSynthParams* synthParams);

/** Starts the sequencer playing. If the sequencer was stopped, it will resume
 * at the last played slot. */
void
Sequencer_start(void);

/** Clears the sequencer. Does not stop! */
void
Sequencer_clear(void);

/** Stops the sequencer. */
void
Sequencer_stop(void);

/** Sets the current slot index to 0 and starts the sequencer. */
void
Sequencer_reset(void);

/**
 * Set the sequencer BPM.
 *
 * @param bpm The new bpm.
 */
void
Sequencer_setBpm(SequencerBpm bpm);

/**
 * Adjust the sequencer BPM to the current bpm + bpmDelta.
 *
 * @param bpm Offset to add to the current bpm.
 * @return SEQ_OK on success, -1 if the delta would result in a new bpm less
 * than 20 or greater than 300.
 */
int
Sequencer_adjustBpm(SequencerBpmDelta bpmDelta);

/**
 * Stops and destroys the sequencer.
 */
void
Sequencer_destroy(void);
