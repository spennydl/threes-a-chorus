/**
 * @file cbuffer.h
 * @brief A lock-free circular buffer over chunks of frames.
 *
 * Allocates a circular buffer of N chunks of M frames. This buffer is safe for
 * use across multiple threads if and only if there is a single producer and a
 * single consumer.
 *
 * Internally, the buffer keeps a read offset and a write offset that point to
 * the beginning of each chunk. No locking is required as reading and writing
 * these indexes is atomic, and the reads and writes are performed through a
 * callback which allows us to know when readers/writers are in a buffer.
 *
 * The implementation was inspired by and adapted from QuantumLeap's
 * implementation available at
 * https://github.com/QuantumLeaps/lock-free-ring-buffer
 *
 * @author Spencer Leslie
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/** Error code for OK. */
#define CBUFFER_EOK 1
/** Error code indicating allocation failure. */
#define CBUFFER_EALLOC -1
/** Error code indicating the buffer is empty. */
#define CBUFFER_ENODATA -2
/** Error code indicating the buffer is full. */
#define CBUFFER_EFULL -3

/**
 * @brief Buffer read/write callback function.
 *
 * Both readers and writers must pass this callback to the respective
 * reading/writing functions. This callback will be called with a pointer to the
 * beginning of the next available chunk and the length of the chunk.
 *
 * It is only safe for readers/writers to access data from the chunk pointer to
 * (chunk pointer + chunk length).
 *
 * @param chunk The chunk pointer.
 * @param chunkLen The length of the chunk.
 */
typedef void (*CBufferCallback)(int16_t* chunk, size_t chunkLen);

/**
 * @brief Allocate and intitialize the CBuffer.
 *
 * The total buffer size will be `chunkSize * nChunks * sizeof(int16_t)` bytes.
 *
 * @param chunkSize The size of the chunks.
 * @param nChunks The number of chunks in the buffer.
 * @return int Status code indicating success or allocation error.
 */
int
CBuffer_new(size_t chunkSize, size_t nChunks);

/**
 * @brief Read the next available chunk.
 *
 * Determines whether data is available for reading and calls the callback with
 * a pointer to the next available unread chunk.
 *
 * @param cb The callback.
 * @return int Either CBUFFER_EOK if the read was performed, or CBUFFER_ENODATA
 * if the buffer is empty.
 */
int
CBuffer_readNextChunk(CBufferCallback cb);

/**
 * @brief Write to the next available chunk.
 *
 * Determines whether there is room in the buffer and calls the callback with
 * a pointer to the next available free chunk.
 *
 * @param cb The callback.
 * @return int Either CBUFFER_EOK if the write was performed, or CBUFFER_EFULL
 * if the buffer is full.
 */
int
CBuffer_writeNextChunk(CBufferCallback cb);

/**
 * @brief Frees the buffer.
 */
void
CBuffer_destroy(void);
