cmake_minimum_required (VERSION 3.21)


add_subdirectory("MDP_Interfaces")
add_subdirectory("NeuralNetworkTrainer")
add_subdirectory("Optimal")
add_subdirectory("AllComponents")

if(${dynaplex_enable_pythonbindings})
add_subdirectory("PythonBindings")
endif()