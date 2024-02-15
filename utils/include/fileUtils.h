/*
 * Provides high-level functions to read and write files
 * without having to manually open, close, or error check.
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

// Reads a file and stores the read value in a provided buffer.
void File_read(const char *filepath, char *buffer);

// Reads a file and directly returns the read int.
int File_readInt(const char *filepath);

void File_write(const char *filepath, const char *writeValue);

#endif