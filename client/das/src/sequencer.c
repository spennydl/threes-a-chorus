/**
 * @file sequencer.c
 * @brief Implementation of the sequencer.
 * @author Spencer Leslie 301571329
 */
#include "das/sequencer.h"
#include "com/timeutils.h"
#include "das/fmplayer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Slots in the sequencer. */
#define SEQUENCER_SLOTS (SEQ_BEAT_SLOTS * SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE)

/** Converts BPM to nanoseconds between slots. */
#define BPM_TO_NS(B) (NS_IN_MINUTE / (B) / SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE)

/** Get a sequencer slot index given quarter, eighth, and sixteenth note index.
 */
#define SEQ_SLOT_IDX(Q, E, S)                                                  \
    (((Q) * SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE) +                              \
     ((E) * SEQ_EIGHTH_NOTE_IN_QUARTER_NOTE) + (S))

/** Number of NS in a minute. */
#define NS_IN_MINUTE 60000000000

/** States the sequencer can be in. */
enum sequencerState
{
    SEQ_RUN,
    SEQ_STOP,
    SEQ_RESET,
    SEQ_END
};

/** Internal sequencer struct. */
struct sequencer
{
    /** The sequence. */
    SequencerOp sequence[SEQUENCER_SLOTS];
    /** Current playback position. */
    SequencerIdx playbackPosition;
    /** BPM we're playing at. */
    SequencerBpm bpm;
    /** Optional callback. */
    loopCallbackFn loopCallback;
    /** ns between advancing one slot (sixteenth note). */
    unsigned long long nsBetweenUpdates;
};

/** The sequencer. */
static struct sequencer* seq;
/** Current sequencer state. */
static _Atomic enum sequencerState _sequencerState = SEQ_STOP;
/** The sequencer worker thread.*/
static pthread_t _sequencerThread;
/** Lock protecting the sequencer data. */
static pthread_rwlock_t _seqLock = PTHREAD_RWLOCK_INITIALIZER;
/** State condition used for pausing the sequencer when stopped. */
static pthread_cond_t _stateCond = PTHREAD_COND_INITIALIZER;
/** Mutex required for the state condition.*/
static pthread_mutex_t _stateCondMutex = PTHREAD_MUTEX_INITIALIZER;

/** Performs the actions in the sequencer slot referenced by the given index. */
static void
_runSequencerSlot(SequencerIdx idx);
/** Main sequencer thread function. */
static void*
_sequencer(void*);

static void
_runSequencerSlot(SequencerIdx idx)
{
    SequencerOp* op = &seq->sequence[idx];
    if (op->synthParams != NULL) {
        FmPlayer_setSynthVoice(op->synthParams);
    }

    if (op->note != NOTE_NONE) {
        FmPlayer_setNote(op->note);
    }

    if (op->op != NOTE_CTRL_NONE) {
        FmPlayer_controlNote(op->op);
    }
}

static void*
_sequencer(void* _data)
{
    (void)_data;
    int state;
    while (1) {
        state = _sequencerState;
        switch (state) {
            case SEQ_RUN: {
                // run the sequencer
                // This thread is the only one that reads/writes playback
                // position
                long long start = Timeutils_getTimeInNs();
                long long timeBetweenUpdates = 0;

                SequencerIdx currentPos = seq->playbackPosition;

                // Call the callback first.
                if (currentPos == 0 && seq->loopCallback) {
                    // TODO: This doesn't allow the user to cancel the
                    // sequencer. We should at least check the state after
                    // calling this to see if it was changed.
                    seq->loopCallback();
                }

                pthread_rwlock_rdlock(&_seqLock);
                _runSequencerSlot(currentPos);
                timeBetweenUpdates = seq->nsBetweenUpdates;
                pthread_rwlock_unlock(&_seqLock);

                if (currentPos + 1 >= SEQUENCER_SLOTS) {
                    seq->playbackPosition = 0;
                } else {
                    seq->playbackPosition = currentPos + 1;
                }

                long long elapsed = Timeutils_getTimeInNs() - start;
                long long toSleep = timeBetweenUpdates - elapsed;
                Timeutils_sleepForNs(toSleep);

                break;
            }
            case SEQ_STOP: {
                FmPlayer_controlNote(NOTE_CTRL_NOTE_OFF);

                pthread_mutex_lock(&_stateCondMutex);
                pthread_cond_wait(&_stateCond, &_stateCondMutex);
                pthread_mutex_unlock(&_stateCondMutex);

                break;
            }
            case SEQ_RESET: {
                seq->playbackPosition = 0;
                _sequencerState = SEQ_RUN;
                break;
            }
            case SEQ_END: {
                return NULL;
                break;
            }
            default: {
                fprintf(stderr, "WARN: sequencer invalid state %d\n", state);
                break;
            }
        }
    }
}

int
Sequencer_initialize(SequencerBpm bpm, loopCallbackFn callback)
{
    seq = malloc(sizeof(struct sequencer));
    if (!seq) {
        return SEQ_EALLOC;
    }

    memset(seq, 0, sizeof(struct sequencer));

    for (SequencerIdx i = 0; i < SEQUENCER_SLOTS; i++) {
        seq->sequence[i].note = NOTE_NONE;
    }

    seq->bpm = bpm;
    seq->nsBetweenUpdates = BPM_TO_NS(bpm);
    seq->loopCallback = callback;

    // TODO: error
    pthread_create(&_sequencerThread, NULL, _sequencer, NULL);

    return SEQ_OK;
}

SequencerIdx
Sequencer_getSlotIndex(int quarter, int eighth, int sixteenth)
{
    return SEQ_SLOT_IDX(quarter, eighth, sixteenth);
}

void
Sequencer_fillSlot(SequencerIdx idx,
                   FmPlayer_NoteCtrl control,
                   Note note,
                   FmSynthParams* synthParams)
{
    pthread_rwlock_wrlock(&_seqLock);
    seq->sequence[idx].op = control;
    seq->sequence[idx].note = note;
    seq->sequence[idx].synthParams = synthParams;
    pthread_rwlock_unlock(&_seqLock);
}

void
Sequencer_start(void)
{
    if (_sequencerState == SEQ_STOP) {
        _sequencerState = SEQ_RUN;
        pthread_cond_signal(&_stateCond);
    }
}

void
Sequencer_stop(void)
{
    _sequencerState = SEQ_STOP;
}

void
Sequencer_clear(void)
{
    for (int i = 0; i < SEQUENCER_SLOTS; i++) {
        seq->sequence[i].note = NOTE_NONE;
        seq->sequence[i].op = NOTE_CTRL_NONE;
        seq->sequence[i].synthParams = NULL;
    }
}

void
Sequencer_reset(void)
{
    _sequencerState = SEQ_RESET;
    pthread_cond_signal(&_stateCond);
}

void
Sequencer_setBpm(SequencerBpm bpm)
{
    seq->nsBetweenUpdates = BPM_TO_NS(bpm);
}

int
Sequencer_adjustBpm(SequencerBpmDelta bpmDelta)
{
    int sign = bpmDelta > 0 ? 1 : -1;
    size_t posBpmDelta = sign * bpmDelta;

    if (sign == -1 && posBpmDelta > seq->bpm) {
        return SEQ_EINVAL;
    }

    seq->bpm += bpmDelta;
    seq->nsBetweenUpdates = BPM_TO_NS(seq->bpm);

    return SEQ_OK;
}

void
Sequencer_destroy(void)
{
    _sequencerState = SEQ_END;
    pthread_cond_signal(&_stateCond);
    pthread_join(_sequencerThread, NULL);

    free(seq);
}
