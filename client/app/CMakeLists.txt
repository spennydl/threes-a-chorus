# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(tac ${MY_SOURCES})

set(THREAD_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Make use of the HAL library
target_link_libraries(tac LINK_PRIVATE Threads::Threads hal das)

add_custom_command(
  TARGET tac
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:tac>"
          "~/cmpt433/public/myApps/tac"
  COMMENT "Copying ARM executable to public NFS directory")
