#include "fileUtils.h"

static FILE *File_open(const char *filepath, const char *mode);
static void File_close(FILE *file);

static FILE *File_open(const char *filepath, const char *mode)
{
    FILE *file = fopen(filepath, mode);
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    return file;
}

static void File_close(FILE *file)
{
    int result = fclose(file);
    if (result == EOF) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }
}

// TODO: Can I use fscanf here too (to generalize the File_read function).
void File_read(const char *filepath, char *buffer)
{
    FILE *file = File_open(filepath, "r");

    fgets(buffer, sizeof(buffer), file);

    File_close(file);
}

int File_readInt(const char *filepath)
{
    int pInt;

    FILE *file = File_open(filepath, "r");

    int itemsRead = fscanf(file, "%d", &pInt);
    if (itemsRead <= 0) {
        perror("Error reading file");
        exit(EXIT_FAILURE);
    }

    File_close(file);

    return pInt;
}

void File_write(const char *filepath, const char *writeValue)
{
    FILE *file = File_open(filepath, "w");

    int charsWritten = fprintf(file, "%s", writeValue);
    if (charsWritten <= 0) {
        perror("Error writing file");
        exit(EXIT_FAILURE);
    }

    File_close(file);
}