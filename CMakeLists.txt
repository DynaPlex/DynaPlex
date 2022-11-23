# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("Library")

#needed to succesfully make a dll on windows. Currently N/A
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_subdirectory("mdp_interface")
add_subdirectory("mdp_opt")
add_subdirectory ("test")

