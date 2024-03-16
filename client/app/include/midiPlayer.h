#pragma once

void
MidiPlayer_initialize();

void
MidiPlayer_cleanup();

void
MidiPlayer_playMidiFile(char* path, int channelNumber);

void
MidiPlayer_playNextBeat();

void
MidiPlayer_setBeatOffset(long long newBeatOffset);

void
MidiPlayer_setBpm(int newBpm);

void*
midiPlayerWorker(void* p);
