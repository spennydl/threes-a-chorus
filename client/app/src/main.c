// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>

#include "tcpExample.h"
#include "ultrasonicExample.h"

int
main(int argc, char** argv)
{
    tcpFileExample(argc, argv);
    return 0;
}