#include "das/sequencer.h"
#include "das/fmplayer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEQUENCER_SLOTS (SEQ_BEAT_SLOTS * SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE)

#define BPM_TO_NS(B) (NS_IN_MINUTE / (B) / SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE)

#define SEQ_SLOT_IDX(Q, E, S)                                                  \
    (((Q) * SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE) +                              \
     ((E) * SEQ_EIGHTH_NOTE_IN_QUARTER_NOTE) + (S))

#define NS_IN_MINUTE 60000000000

enum sequencerState
{
    SEQ_RUN,
    SEQ_STOP,
    SEQ_RESET,
    SEQ_END
};

struct sequencer
{
    SequencerOp sequence[SEQUENCER_SLOTS];
    SequencerIdx playbackPosition;
    SequencerBpm bpm;
    unsigned long long nsBetweenUpdates;
};

static struct sequencer* seq;
static _Atomic enum sequencerState _sequencerState = SEQ_STOP;

static pthread_t _sequencerThread;
static pthread_rwlock_t _seqLock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_cond_t _stateCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t _stateCondMutex = PTHREAD_MUTEX_INITIALIZER;

static void*
_sequencer(void*);

int
Sequencer_initialize(SequencerBpm bpm)
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

static long long
_getTimeInNs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;

    long long totalNanoSeconds = seconds * 1000000000 + nanoSeconds;
    return totalNanoSeconds;
}

static void
_sleepForNs(long long delayNs)
{
    const long long NS_PER_SECOND = 1000000000;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;

    struct timespec reqDelay = { seconds, nanoseconds };
    nanosleep(&reqDelay, (struct timespec*)NULL);
}

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
                long long start = _getTimeInNs();
                long long timeBetweenUpdates = 0;

                SequencerIdx currentPos = seq->playbackPosition;

                pthread_rwlock_rdlock(&_seqLock);
                _runSequencerSlot(currentPos);
                timeBetweenUpdates = seq->nsBetweenUpdates;
                pthread_rwlock_unlock(&_seqLock);

                if (currentPos + 1 >= SEQUENCER_SLOTS) {
                    seq->playbackPosition = 0;
                } else {
                    seq->playbackPosition = currentPos + 1;
                }

                long long elapsed = _getTimeInNs() - start;
                long long toSleep = timeBetweenUpdates - elapsed;
                _sleepForNs(toSleep);

                break;
            }
            case SEQ_STOP: {
                // TODO: Would love to be able to block on a condition without
                // having to hold this unnecessary mutex
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
