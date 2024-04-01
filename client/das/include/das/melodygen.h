#pragma once

#include "das/fm.h"

// c("1", "4", "5", "6", "2", "3", "27", "47", "16", "67", "56", "5/6", "b7",
// "164", "57", "37", "5/5", "464", "17", "664", "564", "46", "57/6", "642",
// "36", "66", "b6", "b3")

typedef enum
{
    I,
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
    VI42,
    iii6,
    vi6,
    bVI,
    bIII
} Chord;

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
    { C3, E3, G3, Bb3 },         // I7
    { E3, A3, C4, NOTE_NONE },   // VI64
    { D3, G3, B3, NOTE_NONE },   // V64
    { C3, E3, A3, NOTE_NONE },   // IV6
    { E3, Ab3, B3, D4 },         // V7a6
    { E3, F3, A3, C4 },          // VI42
    { G3, B3, E4, NOTE_NONE },   // iii6
    { C3, E3, A3, NOTE_NONE },   // vi6
    { Ab3, C3, Eb3, NOTE_NONE }, // bVI
    { Eb3, G3, Bb3, NOTE_NONE }  // bIII
};

static const Note majorScale[] = { C2, D2, E2, F2, G2, A2, B2 };

static const Chord tempProgressions[][4] = { { I, I, V, I } };

void
Melody_generateToSequencer(void);
