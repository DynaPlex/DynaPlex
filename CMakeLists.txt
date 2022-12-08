# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)

project ("Library" CXX)

#needed to succesfully make a dll on windows. Currently N/A
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_subdirectory("DynaPlex")
add_subdirectory("Examples")


