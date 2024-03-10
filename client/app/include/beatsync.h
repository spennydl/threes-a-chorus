/**
 * Init all threads for BeatSync
 *
*/
void
BeatSync_initialize();

/**
 * Clean up threads for BeatSync
*/
void BeatSync_cleanup();

/**
 * A thread to ask the server how long it should wait between beats
*/
void*
beatRequesterWorker(void* p);