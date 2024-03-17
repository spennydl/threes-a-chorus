#define OFFSET_CODE "OFFSET"

/**
 * Init all threads for BeatSync
 *
*/
void
BeatSync_initialize();

/**
 * Clean up threads for BeatSync
*/
void
BeatSync_cleanup();

/**
 * Set the path of the midi file to send to clients when they ask
*/
void
BeatSync_setMidiToSend(char* path);

/**
 * Set a new BPM 
*/
void
BeatSync_setBpm(int newBpm);

/**
 * A thread that sleeps between beats and updates variables saying how long to wait until the next beat
*/
void*
beatSyncThreadWorker(void* p);
