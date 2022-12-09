#PREAMBLE
cmake_minimum_required (VERSION 3.21)
project ("DynaPlex" CXX)

# PROJECT WIDE SETUP

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# DEPENDENCIES
include(cmake/dependencies.cmake)

# SETTINGS
include(dynaplex_settings)
# MAIN TARGETS
add_subdirectory(src/Library)
#include examples unless this project is included as part of bigger project.
if(dynaplex_as_standalone)
add_subdirectory(src/Examples)
endif()

