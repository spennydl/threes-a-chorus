# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(
  threes_a_chorus_project
  VERSION 1.0
  DESCRIPTION "Three's a Chorus Client"
  LANGUAGES C)

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Werror -Wpedantic -Wextra)
add_compile_options(-fdiagnostics-color)

# Enable address sanitizer (Comment this out to make your code faster)
# add_compile_options(-fsanitize=address) add_link_options(-fsanitize=address)

# What folders to build
add_subdirectory(common)
add_subdirectory(hal)
add_subdirectory(das)
add_subdirectory(app)
