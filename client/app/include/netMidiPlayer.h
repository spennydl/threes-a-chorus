#pragma once

typedef int NetMidi_Channel;

int
NetMidi_openMidiChannel(const char* hostname, NetMidi_Channel channel);

void
NetMidi_stop(void);
