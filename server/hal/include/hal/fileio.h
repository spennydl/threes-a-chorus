/**
 * @file fileio.h
 * @brief Helper functions for file IO based on the C file stream functions.
 * @author Spencer Leslie 301571329
 */
#ifndef FILEIO_H
#define FILEIO_H

#include <stdbool.h>
#include <stddef.h>

/* Buffer sizes for convenience. */
#define FILEIO_SMALL_BUFSIZE 64
#define FILEIO_MED_BUFSIZE 1024

/**
 * Tests whether or not a directory exists.
 * @param path The path to the directory to test.
 * @return int 1 if the directory exists, 0 if it does not, and -1 if
 *             an error occurred.
 */
int
fileio_directory_exists(const char* restrict path);

/**
 * Write a string to a file, overwriting its contents.
 * @param path The file to write to.
 * @param value The string to write.
 * @return int The number of characters written or -1 on error.
 */
int
fileio_write(const char* restrict path, const char* restrict value);

/**
 * Read a line of text from a file.
 * @param path The file to read from.
 * @param buf A buffer to write the line to.
 * @param bufsiz The max length of the buffer.
 * @return Whether or not the operation succeeded.
 */
bool
fileio_read_line(const char* restrict path, char* restrict buf, size_t bufsiz);

/**
 * Calls popen to fork/exec a command.
 * @param cmd The command to execute.
 * @return The exit status of the command or -1 on error.
 */
int
fileio_pipe_command(const char* restrict cmd);

#endif
