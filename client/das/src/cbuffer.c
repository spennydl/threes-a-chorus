#include "das/cbuffer.h"

#include <malloc.h>
#include <stddef.h>
#include <stdint.h>

static size_t readIndex = 0;
static size_t writeIndex = 0;

static int16_t* buffer = NULL;
static long bufferChunkSize;

int
CBuffer_new(size_t chunkSize)
{
    bufferChunkSize = chunkSize;
    buffer = calloc(CBUFFER_N_CHUNKS * bufferChunkSize, sizeof(int16_t));
    if (!buffer) {
        return CBUFFER_EALLOC;
    }
    return CBUFFER_EOK;
}

int
CBuffer_readNextChunk(CBufferCallback cb, void* usr)
{
    size_t nextValidIdx = (readIndex + 1 >= 8) ? 0 : readIndex + 1;
    if (nextValidIdx == writeIndex) {
        return CBUFFER_ENODATA;
    }
    readIndex = nextValidIdx;
    cb(buffer + (readIndex * bufferChunkSize), bufferChunkSize, usr);
    return CBUFFER_EOK;
}

int
CBuffer_writeNextChunk(CBufferCallback cb, void* usr)
{
    size_t nextValidIdx = (writeIndex + 1 >= 8) ? 0 : writeIndex + 1;
    if (nextValidIdx == readIndex) {
        return CBUFFER_EFULL;
    }
    writeIndex = nextValidIdx;
    cb(buffer + (readIndex * bufferChunkSize), bufferChunkSize, usr);
    return CBUFFER_EOK;
}

size_t
CBuffer_bufferSize(void)
{
    return bufferChunkSize;
}

void
CBuffer_destroy(void)
{
    free(buffer);
    buffer = NULL;
}
