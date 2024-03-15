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

void
BeatSync_setMidiToSend(char* path);

/**
 * A thread that sleeps between beats and updates variables saying how long to wait until the next beat
*/
void*
beatSyncThreadWorker(void* p);
