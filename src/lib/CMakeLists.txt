cmake_minimum_required (VERSION 3.21)

add_subdirectory("core")
add_subdirectory("nn")
add_subdirectory("optimal")
add_subdirectory("dp")

if(${dynaplex_enable_pythonbindings})
add_subdirectory("bindings")
endif()