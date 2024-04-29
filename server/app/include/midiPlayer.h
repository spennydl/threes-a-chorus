#pragma once
#include "tcp.h"

/**
 * Init all threads
 *
 */
Server_Status
MidiPlayer_initialize();

/**
 * Clean up all threads for BeatSync
 *
 */
void
MidiPlayer_cleanup();

/**
 * Play midi file at path path
 *
 */
Server_Status
MidiPlayer_playMidiFile(char* path);

/**
 * Set the BPM of the midi player (is calculated in BeatSync from the server)
 *
 */
void
MidiPlayer_setBpm(int newBpm);

/**
 * Read the name of a random midi file in midis folder into buffer
 */
void
MidiPlayer_getRandomMidiPath(char* buffer);

void*
midiPlayerWorker(void* p);
