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
 * Play midi file at path path and use channelNumber as the the channel index (is mod by # channels to make sure no overflow)
 *
*/
void
MidiPlayer_playMidiFile(char* path, int channelNumber);

/**
 * Tell the midi player it is allowed to play another beat
 *
*/
void
MidiPlayer_playNextBeat();

/**
 * Set how many beats offset to start playing a midi file at
 *
*/
void
MidiPlayer_setBeatOffset(long long newBeatOffset);

/**
 * Set the BPM of the midi player (is calculated in BeatSync from the server)
 *
*/
void
MidiPlayer_setBpm(int newBpm);

void*
midiPlayerWorker(void* p);
