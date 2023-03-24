#PREAMBLE
cmake_minimum_required (VERSION 3.21)
project ("DynaPlex" CXX)

# PROJECT WIDE SETUP

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)


# DEPENDENCIES
include(cmake/dependencies.cmake)


# SETTINGS
include(dynaplex_settings)

# MAIN TARGETS
add_subdirectory(src/Library)
#include examples unless this project is included as part of bigger project.
if(${PROJECT_IS_TOP_LEVEL})
add_subdirectory(src/Examples)
endif()

#glob directories

#https://www.reddit.com/r/cpp_questions/comments/xnu9e0/cmake_how_to_glob_or_add_all_subdirectories/

#maybe add configure_depends?
#file(GLOB V_GLOB LIST_DIRECTORIES true "*")
#foreach(item ${V_GLOB})
#	if(IS_DIRECTORY ${item})
#		add_subdirectory(${item})
#	endif()
#endforeach()