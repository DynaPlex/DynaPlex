#PREAMBLE
cmake_minimum_required (VERSION 3.21)
project ("DynaPlexSuite" CXX)

# PROJECT WIDE SETUP
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
# DEPENDENCIES
include(cmake/dependencies.cmake)

# SETTINGS
include(dynaplex_settings)
# MAIN TARGETS
add_subdirectory(src/lib)
#include examples unless this project is included as part of bigger project.
if(PROJECT_IS_TOP_LEVEL AND NOT dynaplex_enable_pythonbindings)
add_subdirectory(src/executables)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
if(dynaplex_enable_tests)
enable_testing()
add_subdirectory(src/extern/googletest)
add_subdirectory(src/tests)
endif(dynaplex_enable_tests)
endif()
