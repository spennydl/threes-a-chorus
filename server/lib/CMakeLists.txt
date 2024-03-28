# CMakeList.txt for lib
#   Build a library (`lib`) which exposes the header files as "lib/*.h"
#   Use header as: #include "lib/button.h"

include_directories(lib/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(lib STATIC ${MY_SOURCES})

target_include_directories(lib PUBLIC include)
target_link_libraries(lib m)