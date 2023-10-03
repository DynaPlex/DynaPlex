
cmake_minimum_required (VERSION 3.21)

if(MPI_FOUND)
add_subdirectory(mpi_tests)
endif()
add_subdirectory(memory_checks)
add_subdirectory(executable_example)
add_subdirectory(parametric_policy_opt)