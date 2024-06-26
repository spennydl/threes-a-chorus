/**
 * @file melodygen.c
 * @brief Implementation of melody generation.
 * @author Spencer Leslie 301571329
 */
#include "das/melodygen.h"
#include "das/fm.h"
#include "das/fmplayer.h"
#include "das/sequencer.h"
#include <stdio.h>
#include <stdlib.h>

/** Number of half steps in one octave. */
#define HALF_STEPS_IN_OCTAVE 12

/** Macro that returns the sign of a number. */
#define SIGNOF(X) (((X) >= 0) ? 1 : -1)

/** The last note that was generated. Used for linking melodies together. */
static Note _lastNotePlayed = NOTE_NONE;

/** Generates a random integer in a given inclusive range. */
static int
_randInRange(int start, int end);

/** Performs a random test with the given probablity. */
static bool
_randomTest(const float chance);

/** Gets the absolute signed distance between two notes. */
static int
_halfStepsAway(const Note from, const Note to);

/**
 * Get the distance between two notes using modular arithmetic. This will give
 * the distance independent of the octave, e.g the distance between C3 and D5 is
 * 2, and the distance between C3 and B6 is -1.
 */
static int
_noteSignedRingDistance(const Note from, const Note to);

/**
 * Is the distance in the given direction? e.g. if we want the melody to rise,
 * is the distance positive?
 */
static inline bool
_distanceInRightDirection(int d, int dir);

/** Picks a random note in the given chord. */
static Note
_pickRandomChordNote(const Chord chord);

/**
 * Picks the closest note in the given chord from the given note in the given
 * direction.
 */
static Note
_closestInChord(const Chord chord, const Note from, const int dir);

/**
 * Generates 2 beats of eigth notes to the seqeuncer starting from the given
 * sequencer index that arpeggiate the given chord starting from the given note
 * in the given direction.
 */
static int
_arpeggiateChord(const SequencerIdx startIdx,
                 const Chord chord,
                 const Note from,
                 const int direction);

/** Selects a random chord progression in a major or minor key. */
static const Chord*
_selectChordProgression(const MelodyGenKey key);

/**
 * Picks a passing tone that stays in key starting from the given
 * note moving in the given direction.
 */
static Note
_getPassingTone(const Note from, const int direction);

/** Generates a melody to the sequencer according to the given params. */
static void
_generateToSequencer(const MelodyGenParams* params);

static int
_randInRange(int start, int end)
{
    return (rand() % (end + 1 - start)) + start;
}

static bool
_randomTest(const float chance)
{
    return ((float)rand() / RAND_MAX) < chance;
}

static int
_halfStepsAway(const Note from, const Note to)
{
    return to - from;
}

static int
_noteSignedRingDistance(const Note from, const Note to)
{
    int fromBase = from % HALF_STEPS_IN_OCTAVE;
    int toBase = to % HALF_STEPS_IN_OCTAVE;

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
_pickRandomChordNote(const Chord chord)
{
    Note result = NOTE_NONE;
    const Note* notesInChord = chordTable[chord];

    while (result == NOTE_NONE) {
        result = notesInChord[_randInRange(0, 3)];
    }
    return result;
}

static Note
_closestInChord(const Chord chord, const Note from, const int dir)
{
    if (from == NOTE_NONE) {
        return _pickRandomChordNote(chord);
    }

    Note result = NOTE_NONE;
    const Note* notesInChord = chordTable[chord];

    int distance = INT32_MAX;
    for (int i = 0; i < 4; i++) {

        Note chordNote = notesInChord[i];
        if (notesInChord[i] == NOTE_NONE) {
            continue;
        }

        int d = _noteSignedRingDistance(from, chordNote);
        int absd = abs(d);
        if (d == 0 &&
            !_randInRange(0, 2)) { // TODO: this chance could be adjusted
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

static int
_arpeggiateChord(const SequencerIdx startIdx,
                 const Chord chord,
                 const Note from,
                 const int direction)
{
    Note currentNote = _closestInChord(chord, from, direction);
    Note first = currentNote % 12;

    const Note* notesInChord = chordTable[chord];

    Sequencer_fillSlot(startIdx, NOTE_CTRL_NOTE_ON, currentNote, NULL);

    int i = 0;
    int step = 1;
    if (direction < 0) {
        i = 3;
        step = -1;
    }

    int beatIdx = 1;
    for (; i < 4 && i >= 0; i += step) {
        Note normedChord = notesInChord[i] % 12;
        if (notesInChord[i] == NOTE_NONE || normedChord == first) {
            continue;
        }

        currentNote += _noteSignedRingDistance(currentNote, notesInChord[i]);
        Sequencer_fillSlot(startIdx + (beatIdx * 2), // eighth notes
                           NOTE_CTRL_NOTE_ON,
                           currentNote,
                           NULL);
        beatIdx++;
    }
    _lastNotePlayed = currentNote;
    return beatIdx;
}

static const Chord*
_selectChordProgression(const MelodyGenKey key)
{
    int idx = _randInRange(0, 4);
    return (key == KEY_MAJOR) ? majorProgressions[idx] : minorProgressions[idx];
}

static Note
_getPassingTone(const Note from, const int direction)
{
    Note stripped = from % 12;

    int noteIdx;
    for (noteIdx = 0; noteIdx < 7; noteIdx++) {
        if (majorScale[noteIdx] == stripped) {
            break;
        }
    }

    noteIdx += direction;
    if (noteIdx < 0) {
        noteIdx += 7;
    } else if (noteIdx >= 7) {
        noteIdx -= 7;
    }

    return from + _noteSignedRingDistance(stripped, majorScale[noteIdx]);
}

static void
_generateToSequencer(const MelodyGenParams* params)
{
    Sequencer_setBpm(params->tempo);

    const Chord* prog = _selectChordProgression(params->key);

    Note currentNote = NOTE_NONE;

    for (int i = 0; i < 4; i++) { // for each beat
        int beatIdx = Sequencer_getSlotIndex(i * 2, 0, 0);

        // bring us back if we got too high or low
        if (_lastNotePlayed != NOTE_NONE &&
            (_lastNotePlayed < 0 || _lastNotePlayed > B6)) {
            _lastNotePlayed = _closestInChord(prog[i], NOTE_NONE, 0);
        }

        int direction = _randomTest(params->upDownTendency) ? 1 : -1;
        bool jumpy = _randomTest(params->jumpChance);
        bool dense = _randomTest(params->noteDensity);
        FmPlayer_NoteCtrl noteCtrl = _randomTest(params->stoccatoLegatoTendency)
                                       ? NOTE_CTRL_NOTE_STOCCATO
                                       : NOTE_CTRL_NOTE_ON;

        // on-beat
        if (jumpy) {
            if (dense && _randomTest(0.5)) {
                int notesAdded = _arpeggiateChord(
                  beatIdx, prog[i], _lastNotePlayed, direction);
                if (notesAdded < 4) {
                    Note passingTone =
                      _getPassingTone(_lastNotePlayed, direction);

                    Sequencer_fillSlot(
                      beatIdx + 6, noteCtrl, passingTone, NULL);
                    _lastNotePlayed = passingTone;
                }
                // we're full up
                continue;
            } else {
                // furthest up = closest down and vice versa
                if (_randomTest(0.75)) {
                    currentNote =
                      _closestInChord(prog[i], _lastNotePlayed, -direction);
                    if (_lastNotePlayed != NOTE_NONE) {
                        currentNote += HALF_STEPS_IN_OCTAVE * direction;
                    }
                    Sequencer_fillSlot(beatIdx, noteCtrl, currentNote, NULL);
                }
            }
        } else {
            if (dense || _randomTest(0.75)) {
                currentNote =
                  _closestInChord(prog[i], _lastNotePlayed, direction);
                Sequencer_fillSlot(beatIdx, noteCtrl, currentNote, NULL);
            }
        }

        // fill the rest of the 2 beats
        for (int j = 1; j < 4; j++) {
            // try to keep passing tones on off-beats and chord notes on
            // on-beats
            if (_randomTest(params->noteDensity)) {
                currentNote =
                  (j % 2 == 0)
                    ? _closestInChord(prog[i], currentNote, direction)
                    : _getPassingTone(currentNote, direction);
                Sequencer_fillSlot(
                  beatIdx + (j * 2), noteCtrl, currentNote, NULL);
            } else {
                Sequencer_fillSlot(
                  beatIdx + (j * 2), NOTE_CTRL_NOTE_OFF, NOTE_NONE, NULL);
            }
        }
        _lastNotePlayed = currentNote;
    }
}

void
Melody_generateToSequencer(const MelodyGenParams* params)
{
    _generateToSequencer(params);
}
