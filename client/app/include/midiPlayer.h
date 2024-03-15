#pragma once

void
MidiPlayer_initialize();

void
MidiPlayer_cleanup();

void
MidiPlayer_playMidiFile(char* path, int channelNumber);

void
MidiPlayer_canPlayNextBeat();

void*
midiPlayerWorker(void* p);
