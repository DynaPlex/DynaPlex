message("inclusion switches (CMakeUserPresets.json):")
message("dynaplex_enable_pytorch: "${dynaplex_enable_pytorch})
message("dynaplex_enable_gurobi: "${dynaplex_enable_gurobi})


if(${dynaplex_use_pytorch})	
find_package(Torch REQUIRED)
if(Torch_FOUND)
 #  target_compile_definitions(dynaplex PRIVATE Torch_available=0)
 #  target_link_libraries(dynaplex PRIVATE "${TORCH_LIBRARIES}")
 if (MSVC)
 # file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
 # add_custom_command(TARGET dynaplex
  #                   POST_BUILD
  #                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
  #                   ${TORCH_DLLS}
  #                   $<TARGET_FILE_DIR:dynaplex>)
  #set_target_properties(dynaplex PROPERTIES COMPILE_FLAGS "/wd4819 /wd4624")
endif (MSVC)
endif()

if(NOT Torch_FOUND)
 message(STATUS "Torch not found by dynaplex, will not be able to use DLR") 
 message(STATUS "-specify location of torch while calling cmake, or in cmakelist.txt")
endif()

endif()