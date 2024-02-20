# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(tac_server ${MY_SOURCES})

set(THREAD_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Make use of the HAL library
target_link_libraries(tac_server LINK_PRIVATE hal Threads::Threads)

# Copy executable to final location
add_custom_command(TARGET tac_server POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:tac_server>"
     "~/cmpt433/public/myApps/tac_server" 
  COMMENT "Copying executable to public NFS directory")
