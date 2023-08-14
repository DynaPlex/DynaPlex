
cmake_minimum_required (VERSION 3.21)
#bundles all the static libraries into a single dll that the executables 
#and pybind11 bindings can link against. 
set(targetname DynaPlex)


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")
file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "*.h")


add_library (DP_${targetname} SHARED)
add_library (DynaPlex::${targetname} ALIAS DP_${targetname})

set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD 20)
set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD_REQUIRED true)
set_property(TARGET DP_${targetname} PROPERTY CXX_EXTENSIONS off)

set_target_properties(DP_${targetname} PROPERTIES 
	OUTPUT_NAME DynaPlex	
    #OUTPUT_NAME DynaPlex${targetname}
	EXPORT_NAME ${targetname}
)
target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${private_headers} ${sources})



target_include_directories(DP_${targetname} 
PUBLIC 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Core)
target_link_libraries(DP_${targetname} PUBLIC DynaPlex::DP_Torch)
target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Optimal)



if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
endif()

if(${dynaplex_enable_pythonbindings})
 add_custom_command(TARGET DP_${targetname} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/python/dp_lib/libs/"
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DP_${targetname}>
                   ${PROJECT_SOURCE_DIR}/python/dp_lib/libs/
                   COMMENT "Copying DynaPlex shared library to python/dp_lib/libs/")
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