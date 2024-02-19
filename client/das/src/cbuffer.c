#include "das/cbuffer.h"

#include <malloc.h>
#include <stddef.h>
#include <stdint.h>

/** The read index. */
static size_t readIndex = 0;
/** The write index. */
static size_t writeIndex = 0;

/**
 * The buffer. Will be bufferChunkSize * nChunks * sizeof(int16_t) bytes
 * long.
 */
static int16_t* buffer = NULL;
/** How big the chunks are. */
static size_t bufferChunkSize;
/** How many chunks there are. */
static size_t nChunks;

/** Get the next valid chunk index for the buffer. */
static inline size_t
_getNextValidChunkIdx(size_t idx);

static inline size_t
_getNextValidChunkIdx(size_t idx)
{
    return (idx + 1 >= nChunks) ? 0 : idx + 1;
}

int
CBuffer_new(size_t chunkSize, size_t nc)
{
    bufferChunkSize = chunkSize;
    nChunks = nc;
    buffer = calloc(nChunks * bufferChunkSize, sizeof(int16_t));
    if (!buffer) {
        return CBUFFER_EALLOC;
    }
    return CBUFFER_EOK;
}

int
CBuffer_readNextChunk(CBufferCallback cb)
{
    size_t rdIdx = readIndex; // read the read index (atomic)

    if (rdIdx == writeIndex) { // Check if buffer is empty
        return CBUFFER_ENODATA;
    } // write index can only move forward and stops at readIndex - 1,
      // so we are safe to increment

    cb(buffer + (rdIdx * bufferChunkSize), bufferChunkSize);
    readIndex = _getNextValidChunkIdx(rdIdx);

    return CBUFFER_EOK;
}

int
CBuffer_writeNextChunk(CBufferCallback cb)
{
    size_t wrIdx = writeIndex; // read the write index (atomic)

    // Is the next chunk where the reader is?
    size_t nextValidIdx = _getNextValidChunkIdx(wrIdx);
    if (nextValidIdx == readIndex) {
        return CBUFFER_EFULL;
    }

    // write and then advance
    cb(buffer + (wrIdx * bufferChunkSize), bufferChunkSize);
    writeIndex = nextValidIdx;

    return CBUFFER_EOK;
}

void
CBuffer_destroy(void)
{
    free(buffer);
    buffer = NULL;
}
