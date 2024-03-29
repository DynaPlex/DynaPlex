
cmake_minimum_required (VERSION 3.21)
#bundles all the object files into a single dll that the executables can link against. 
set(targetname DynaPlex)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")


if(dynaplex_enable_pythonbindings)
target_sources(DP_Bindings PUBLIC ${headers} PRIVATE ${sources} )
target_include_directories(DP_Bindings PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )


if(DYNAPLEX_IO_ROOT_DIR)
message(STATUS "Providing dynaplex IO root dir: ${DYNAPLEX_IO_ROOT_DIR}")
target_compile_definitions(DP_Bindings PRIVATE DYNAPLEX_IO_ROOT_DIR=\"${DYNAPLEX_IO_ROOT_DIR}\")
endif()

else()
add_library (DP_${targetname} SHARED)
   
target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${sources})
add_library (DynaPlex::${targetname} ALIAS DP_${targetname})
set_target_properties(DP_${targetname} PROPERTIES OUTPUT_NAME DynaPlex	EXPORT_NAME ${targetname})
target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Core DynaPlex::NN  DynaPlex::Models DynaPlex::Utilities DynaPlex::Algorithms)
if(dynaplex_all_warnings)
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3>  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra> )
endif()

install(TARGETS DP_${targetname} DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)


if(Torch_FOUND)
 if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  
  file(GLOB TORCH_LIBS "${TORCH_INSTALL_PREFIX}/lib/*.lib")
  add_custom_command(TARGET DP_${targetname}
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:DP_${targetname}>)

install(FILES ${TORCH_DLLS} DESTINATION bin)
install(FILES ${TORCH_LIBS} DESTINATION bin)
set_target_properties(DP_${targetname} PROPERTIES COMPILE_FLAGS "/wd4819 /wd4624")
set_target_properties(DP_${targetname} PROPERTIES COMPILE_FLAGS "/wd4702")
endif (MSVC)
endif()

if(DYNAPLEX_IO_ROOT_DIR)
message(STATUS "Providing dynaplex IO root dir: ${DYNAPLEX_IO_ROOT_DIR}")
target_compile_definitions(DP_${targetname} PRIVATE DYNAPLEX_IO_ROOT_DIR=\"${DYNAPLEX_IO_ROOT_DIR}\")
endif()

if(MPI_FOUND)
target_compile_definitions(DP_${targetname} PRIVATE DP_MPI_AVAILABLE=1)
target_link_libraries(DP_${targetname} PUBLIC MPI::MPI_CXX)
endif()

endif()