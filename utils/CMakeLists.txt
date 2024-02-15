# CMakeList.txt for UTILS
#   Build a library (`utils`).

include_directories(utils/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(utils STATIC ${MY_SOURCES})

target_include_directories(utils PUBLIC include)
