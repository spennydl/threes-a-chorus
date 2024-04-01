#include "das/melodygen.h"
#include "das/fm.h" // TODO: should move notes into melodygen.h. Or maybe a musicdata.h?
#include "das/fmplayer.h"
#include "das/sequencer.h"
#include <stdio.h>
#include <stdlib.h>

#define HALF_STEPS_IN_OCTAVE 12

static int
randInRange(int start, int end) // half-open! [start, end)
{
    float randNorm = (float)rand() / RAND_MAX;
    int range = end - start;
    return (randNorm * range) + start;
}

static int
_halfStepsAway(const Note from, const Note to)
{
    return (int)to - (int)from;
}

#define SIGNOF(X) (((X) >= 0) ? 1 : -1)

static int
_noteSignedRingDistance(const Note from, const Note to)
{
    int fromBase = (int)from % HALF_STEPS_IN_OCTAVE;
    int toBase = (int)to % HALF_STEPS_IN_OCTAVE;

    int dist = _halfStepsAway(fromBase, toBase);
    int absdist = abs(dist);

    return (absdist < (HALF_STEPS_IN_OCTAVE / 2))
             ? dist
             : SIGNOF(fromBase - toBase) * (HALF_STEPS_IN_OCTAVE - absdist);
}

static inline bool
_distanceInRightDirection(int d, int dir)
{
    return (dir == 0 || SIGNOF(d) == SIGNOF(dir));
}

static Note
_closestInChord(const Chord chord, const Note from, const int dir)
{
    Note result = NOTE_NONE;
    const Note* notesInChord = chordTable[chord];

    if (from == NOTE_NONE) {
        while (result == NOTE_NONE) {
            result = notesInChord[randInRange(0, 4)];
        }
        return result;
    }

    int distance = INT32_MAX;
    for (int i = 0; i < 4; i++) {

        Note chordNote = notesInChord[i];
        if (notesInChord[i] == NOTE_NONE) {
            continue;
        }

        int d = _noteSignedRingDistance(from, chordNote);
        int absd = abs(d);
        if (d == 0 &&
            !randInRange(0, 3)) { // TODO: this chance could be adjusted
            // we share a note, and we are staying on it
            return from;
        } else if (_distanceInRightDirection(d, dir) && absd < distance) {
            distance = absd;
            result = from + d;
        }
    }

    if (result == NOTE_NONE) {
        fprintf(stderr, "WARN: note wasn't found\n");
    }
    return result;
}

void
Melody_generateToSequencer(void)
{
    const Chord* prog = tempProgressions[0];

    Note startingNote = NOTE_NONE;

    const Note* firstChord = chordTable[prog[0]];
    while (startingNote == NOTE_NONE) {
        startingNote = firstChord[randInRange(0, 3)];
    }

    Note currentNote = startingNote;
    int i;
    for (i = 0; i < 3; i++) {
        Sequencer_fillSlot(Sequencer_getSlotIndex(i * 2, 0, 0),
                           NOTE_CTRL_NOTE_STOCCATO,
                           currentNote,
                           NULL);

        // TODO: Clean this up as done with note in chord fn
        int mod = SIGNOF(randInRange(-2, 2));
        // if (mod != 0) {
        Note stripped = currentNote % 12;
        int oct = currentNote / 12;

        int idx = 0;
        for (int j = 0; j < 7; j++) {
            if (majorScale[j] == stripped) {
                idx = j;
            }
        }

        idx += mod;
        if (idx < 0) {
            idx += 7;
            oct -= 1;
        }

        if (idx >= 7) {
            idx -= 7;
            oct += 1;
        }

        currentNote = majorScale[idx] + (oct * 12);

        Sequencer_fillSlot(Sequencer_getSlotIndex((i * 2) + 1, 0, 0),
                           NOTE_CTRL_NOTE_STOCCATO,
                           currentNote,
                           NULL);
        //}

        // get the next chord and find the closest note
        if (randInRange(0, 2)) {
            printf("going FURTHEST UP\n");
            // try furthest up in chord, which equals closest down + 12
            currentNote = _closestInChord(prog[i + 1], currentNote, -1) + 12;
        } else {
            printf("going FURTHEST DOWN\n");
            currentNote = _closestInChord(prog[i + 1], currentNote, 1);
        }
    }
    // add the last one
    Sequencer_fillSlot(Sequencer_getSlotIndex(i * 2, 0, 0),
                       NOTE_CTRL_NOTE_STOCCATO,
                       currentNote,
                       NULL);
}
