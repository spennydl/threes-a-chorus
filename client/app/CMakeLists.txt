# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(tac ${MY_SOURCES})

set(THREAD_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(bbgAlsa STATIC IMPORTED)
set_target_properties(
  bbgAlsa PROPERTIES IMPORTED_LOCATION
                     "${CMAKE_SOURCE_DIR}/bin/libasound.so.2.0.0")

target_link_libraries(
  tac
  LINK_PRIVATE
  lib
  hal
  das
  com
  bbgAlsa
  Threads::Threads)

add_custom_command(
  TARGET tac
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:tac>"
          "~/cmpt433/public/myApps/tac"
  COMMENT "Copying ARM
 executable to public NFS directory")
