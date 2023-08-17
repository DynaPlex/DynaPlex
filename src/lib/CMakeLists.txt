cmake_minimum_required (VERSION 3.21)



if(${dynaplex_enable_pythonbindings})
add_subdirectory("bindings")
endif()


add_subdirectory("core")
add_subdirectory("nn")
add_subdirectory("factories")
add_subdirectory("dp")
