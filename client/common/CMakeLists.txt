# CMakeList.txt for common lib
#
# Build a library (`common`) which exposes the header files as "com/*.h" Use
# header as: #include "com/fileio.h"

include_directories(com/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(com STATIC ${MY_SOURCES})

target_include_directories(com PUBLIC include)
