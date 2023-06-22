cmake_minimum_required (VERSION 3.21)

add_subdirectory("NeuralNetworkTrainer")
add_subdirectory("Optimal")
add_subdirectory("Core")

add_subdirectory("AllComponents")

if(${dynaplex_enable_pythonbindings})
add_subdirectory("PythonBindings")
endif()