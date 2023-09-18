cmake_minimum_required (VERSION 3.21)



if(dynaplex_enable_pythonbindings)
add_subdirectory("bindings")
endif()

add_subdirectory("core")
add_subdirectory("models")
add_subdirectory("nn")
add_subdirectory("utilities")
add_subdirectory("dp")
