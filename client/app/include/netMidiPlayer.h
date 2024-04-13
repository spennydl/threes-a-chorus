#pragma once

typedef int NetMidi_Channel;

/**
 * Send a file response to a message from socketFd
 * @param hostname The ip of the server
 * @param channel The channel you want to subscribe to (really just an int)
 * @return int Return 0 if successful, < 0 if not
*/
int
NetMidi_openMidiChannel(const char* hostname, NetMidi_Channel channel);

/**
 * Shut down NetMidi and stop subscribing to events. Used when taken off an RFID tag.
*/
void
NetMidi_stop(void);
