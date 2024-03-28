# CMakeList.txt for Digital Audio Synthesis library Build a library (`das`)
# which exposes the header files as "das/*.h" Use header as: #include
# "das/button.h"

include_directories(das/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(das STATIC ${MY_SOURCES})

target_include_directories(das PUBLIC include
                                      "${CMAKE_SOURCE_DIR}/common/include")
target_link_libraries(das m)
