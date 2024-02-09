# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(tac_server ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(tac_server LINK_PRIVATE hal)

# Copy executable to final location
add_custom_command(TARGET tac_server POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:tac_server>"
     "~/cmpt433/public/myApps/tac_server" 
  COMMENT "Copying executable to public NFS directory")
