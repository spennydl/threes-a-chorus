include_directories(include)
file(GLOB MY_SOURCES "src/*.c")

add_library(lib STATIC ${MY_SOURCES})

target_include_directories(lib PUBLIC include)
target_link_libraries(lib m)
