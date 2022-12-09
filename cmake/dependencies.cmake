message("inclusion switches (CMakeUserPresets.json):")
message("dynaplex_enable_pytorch: ${dynaplex_enable_pytorch}")
message("dynaplex_enable_gurobi: ${dynaplex_enable_gurobi}")


if(${dynaplex_enable_pytorch})	
find_package(Torch QUIET)
 if(Torch_FOUND)
 message(STATUS "Succesfully found Torch")
else()
 message(STATUS "Torch not found by dynaplex, even though it was requested via dynaplex_enable_pytorch flag. ") 
 message(STATUS "Please specify location of torch in CMAKE_PREFIX_PATH (for your current configuration) in CMakeUserPresets.json (*), or set dynaplex_enable_pytorch to FALSE ")
 message(STATUS "(*) this file should be located in the DynaPlex folder, but might be invisible in some IDEs")
 message(FATAL_ERROR "Torch requested but not provided")
 endif()

else()
 message(STATUS "Torch DISABLED : you will not be able to use neural networks. ") 
endif()
