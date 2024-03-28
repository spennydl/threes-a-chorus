#pragma once

/**
 * Init all threads
 *
*/
void
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
void
MidiPlayer_playMidiFile(char* path);

/**
 * Set the BPM of the midi player (is calculated in BeatSync from the server)
 *
*/
void
MidiPlayer_setBpm(int newBpm);

void*
midiPlayerWorker(void* p);
