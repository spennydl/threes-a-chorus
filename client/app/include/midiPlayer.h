#pragma once

void
MidiPlayer_initialize();

void
MidiPlayer_cleanup();

void
MidiPlayer_playMidiFile(char* path);

void*
midiPlayerWorker(void* p);