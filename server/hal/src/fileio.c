/**
 * @file fileio.c
 * @brief Helper functions for file IO based on the C file streams functions.
 * @author Spencer Leslie 301571329
 */
#include "hal/fileio.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

int
fileio_directory_exists(const char* restrict path)
{
    DIR* dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 1;
    } else if (ENOENT == errno) {
        return 0;
    } else {
        return -1;
    }
}

int
fileio_write(const char* restrict path, const char* restrict value)
{
    FILE* file = fopen(path, "w");
    if (!file) {
        return -1;
    }

    int nchars = fputs(value, file);
    fclose(file);
    return nchars;
}

bool
fileio_read_line(const char* restrict path, char* restrict buf, size_t bufsiz)
{
    FILE* file = fopen(path, "r");
    if (!file) {
        return false;
    }

    if (!fgets(buf, bufsiz, file)) {
        fclose(file);
        return false;
    }
    fclose(file);
    return true;
}

int
fileio_pipe_command(const char* restrict cmd)
{
    // The following is adapted from the assignment spec.
    // call config-pin to tell bbg we're using it for gpio
    // Execute the shell command (output into pipe)
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[FILEIO_MED_BUFSIZE];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
        // printf("--> %s", buffer); // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    return WEXITSTATUS(pclose(pipe));
}
