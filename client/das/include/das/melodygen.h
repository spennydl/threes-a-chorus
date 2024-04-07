#pragma once

#include "das/fm.h"
#include "singer.h"

/** A fast bpm. */
#define TEMPO_FAST 180
/** Slow bpm. */
#define TEMPO_SLOW 60
/** Medium bpm. */
#define TEMPO_MEDIUM 120

/** Major and minor keys. */
typedef enum
{
    KEY_MAJOR,
    KEY_MINOR
} MelodyGenKey;

/** Parameters that influence melody generation. */
typedef struct
{
    /** Tempo in bpm. */
    int tempo;
    /** Key. Major or minor. */
    MelodyGenKey key;
    /** How "jumpy" the melody is. In [0.0 1.0]. */
    float jumpChance;
    /** How dense the melody should be. In [0.0 1.0] */
    float noteDensity;
    /** The tendency for the melody to rise or fall. In [0.0 1.0]. 0.5 is
     * neutral.*/
    float upDownTendency;
    /** Tendancy to choose stoccato (short and punchy) notes or legato (long and
     * flowing) notes.*/
    float stoccatoLegatoTendency;
} MelodyGenParams;

/** Possible chords in roman numeral notation. */
typedef enum
{
    I = 0,
    IV,
    V,
    vi,
    ii,
    iii,
    ii7,
    IV7,
    I6,
    vi7,
    V6,
    Va6,
    bVII,
    I64,
    V7,
    iii7,
    VaV,
    IV64,
    I7,
    vi64,
    V64,
    IV6,
    V7a6,
    IV42,
    iii6,
    vi6,
    bVI,
    bIII
} Chord;

/** Table of notes in each chord. */
static const Note chordTable[][4] = {
    { C3, E3, G3, NOTE_NONE },   // I
    { F3, A3, C3, NOTE_NONE },   // IV
    { G3, B3, D4, NOTE_NONE },   // V
    { A2, C3, E3, NOTE_NONE },   // vi
    { D3, F3, A3, NOTE_NONE },   // ii
    { E3, G3, B3, NOTE_NONE },   // iii
    { D3, F3, A3, C3 },          // ii7
    { F3, A3, C4, E4 },          // IV7
    { E3, G3, C4, NOTE_NONE },   // I6
    { A2, C3, E3, G3 },          // vi7
    { B2, D3, B3, NOTE_NONE },   // V6
    { E3, Ab3, B3, NOTE_NONE },  // Va6
    { Bb2, D3, F3, NOTE_NONE },  // bVII
    { G3, C3, E3, NOTE_NONE },   // I64
    { G3, B3, D4, F4 },          // V7
    { E3, G3, B3, D4 },          // iii7
    { D3, Gb3, A3, C4 },         // VaV
    { A2, C3, F3, NOTE_NONE },   // IV64
    { C3, E3, G3, B3 },          // I7
    { E3, A3, C4, NOTE_NONE },   // VI64
    { D3, G3, B3, NOTE_NONE },   // V64
    { C3, E3, A3, NOTE_NONE },   // IV6
    { E3, Ab3, B3, D4 },         // V7a6
    { E3, F3, A3, C4 },          // IV42
    { G3, B3, E4, NOTE_NONE },   // iii6
    { C3, E3, A3, NOTE_NONE },   // vi6
    { Ab3, C3, Eb3, NOTE_NONE }, // bVI
    { Eb3, G3, Bb3, NOTE_NONE }  // bIII
};

/** Notes in the major scale. */
static const Note majorScale[] = { C2, D2, E2, F2, G2, A2, B2 };

/** Chord progressiosn to generate from */
static const Chord majorProgressions[][4] = { { I, IV, V, I },
                                              { I, vi, ii, V },
                                              { I6, IV, I64, V },
                                              { I6, ii7, I64, V },
                                              { I, bIII, ii7, V64 } };

static const Chord minorProgressions[][4] = { { vi, ii, Va6, vi },
                                              { vi, V, I, Va6 },
                                              { vi, ii, bVI, Va6 },
                                              { vi6, I7, IV7, Va6 },
                                              { ii, IV42, Va6, vi } };

/** Params that serve as a baseline for a mood, and can be altered further by
 * multiplying mood magnitude. */

// TODO: Adjust these params to fit your vision. I did this roughly.

// faster tempo, jumpy melody, major key
static const MelodyGenParams happyParams = { .tempo = TEMPO_FAST,
                                             .key = KEY_MAJOR,
                                             .jumpChance = 0.7,
                                             .noteDensity = 0.7,
                                             .upDownTendency = 0.6,
                                             .stoccatoLegatoTendency = 0.7 };

// slower tempo, longer notes
static const MelodyGenParams sadParams = { .tempo = TEMPO_SLOW,
                                           .key = KEY_MINOR,
                                           .jumpChance = 0.2,
                                           .noteDensity = 0.3,
                                           .upDownTendency = 0.4,
                                           .stoccatoLegatoTendency = 0.1 };

// medium tempo, ascending melody
static const MelodyGenParams angryParams = { .tempo = TEMPO_MEDIUM,
                                             .key = KEY_MINOR,
                                             .jumpChance = 0.3,
                                             .noteDensity = 1.0,
                                             .upDownTendency = 1.0,
                                             .stoccatoLegatoTendency = 0.5 };

// glitchy, random, amelodic sounds
static const MelodyGenParams overstimulatedParams = { .tempo = TEMPO_FAST,
                                                      .key = KEY_MINOR,
                                                      .jumpChance = 0.9,
                                                      .noteDensity = 0.1,
                                                      .upDownTendency = 0.5,
                                                      .stoccatoLegatoTendency =
                                                        0.5 };

// sparse drone/boops
static const MelodyGenParams neutralParams = { .tempo = TEMPO_MEDIUM,
                                               .key = KEY_MAJOR,
                                               .jumpChance = 0.5,
                                               .noteDensity = 0.3,
                                               .upDownTendency = 0.5,
                                               .stoccatoLegatoTendency = 0.5 };

/**
 * Generates a melody according to the mood.
 */
void
Melody_playMelody(const Mood* mood);

void
Melody_generateToSequencer(const MelodyGenParams* params);