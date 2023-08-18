
cmake_minimum_required (VERSION 3.21)
#bundles all the object files into a single dll that the executables can link against. 
set(targetname DynaPlex)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")

add_library (DP_${targetname} SHARED)

target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${sources})
add_library (DynaPlex::${targetname} ALIAS DP_${targetname})

set_target_properties(DP_${targetname} PROPERTIES 
	OUTPUT_NAME DynaPlex	
	EXPORT_NAME ${targetname}
)

target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Core DynaPlex::NN  DynaPlex::Models)


if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3>  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra> )
endif()


if(Torch_FOUND)
 if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET DP_${targetname}
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:DP_${targetname}>)

set_target_properties(DP_${targetname} PROPERTIES COMPILE_FLAGS "/wd4819 /wd4624")
set_target_properties(DP_${targetname} PROPERTIES COMPILE_FLAGS "/wd4702")
endif (MSVC)
endif()