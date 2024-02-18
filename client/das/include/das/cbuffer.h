#pragma once

#include <stddef.h>
#include <stdint.h>

// 4k buffer chunks (at least to start with)
#define CBUFFER_N_CHUNKS 8

#define CBUFFER_EOK 1
#define CBUFFER_EALLOC -1
#define CBUFFER_ENODATA -2
#define CBUFFER_EFULL -3

typedef void (*CBufferCallback)(int16_t* chunk, size_t chunkLen, void* usrData);

int
CBuffer_new(size_t chunkSize);

int
CBuffer_readNextChunk(CBufferCallback cb, void* usr);

int
CBuffer_writeNextChunk(CBufferCallback cb, void* usr);

void
CBuffer_destroy(void);
