cmake_minimum_required (VERSION 3.20)

project ("DynaPlex" CXX)

#needed to succesfully make a dll on windows. Currently N/A
#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_subdirectory("DynaPlex")
add_subdirectory("Examples")

