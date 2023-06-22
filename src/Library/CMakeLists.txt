cmake_minimum_required (VERSION 3.21)


add_subdirectory("DP_Core")
add_subdirectory("DP_Torch")
add_subdirectory("DP_Optimal")

add_subdirectory("DP")

if(${dynaplex_enable_pythonbindings})
add_subdirectory("DP_Bindings")
endif()