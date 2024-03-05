#pragma once

#include "fm.h"

// TODO: triplets?
#define SEQ_EIGHTH_NOTE_IN_QUARTER_NOTE 2
#define SEQ_SIXTEENTH_NOTE_IN_QUARTER_NOTE 4
// TODO: thirtysecond note might be overkill
#define SEQ_THIRTYSECOND_NOTE 8

#define SEQ_BEAT_SLOTS 8

#define SEQ_OK 0
#define SEQ_EALLOC -1
#define SEQ_EINVAL -2

typedef enum
{
    SEQ_NONE = 0,
    SEQ_NOTE_ON,
    SEQ_NOTE_OFF,
    SEQ_NOTE_STOCCATO,
    SEQ_NOTE_LEGATO,
} SequencerControl;

typedef struct
{
    SequencerControl op;
    Note note;
    FmSynthParams* synthParams;
} SequencerOp;

typedef size_t SequencerIdx;
typedef size_t SequencerBpm;
typedef int SequencerBpmDelta;

int
Sequencer_initialize(SequencerBpm bpm);

SequencerIdx
Sequencer_getSlotIndex(int quarter, int eighth, int sixteenth);

void
Sequencer_fillSlot(SequencerIdx idx,
                   SequencerControl control,
                   Note note,
                   FmSynthParams* synthParams);

void
Sequencer_updateSynthParams(FmSynthParams* params);

void
Sequencer_start(void);

void
Sequencer_stop(void);

void
Sequencer_reset(void);

void
Sequencer_setBpm(SequencerBpm bpm);

int
Sequencer_adjustBpm(SequencerBpmDelta bpmDelta);

void
Sequencer_destroy(void);
