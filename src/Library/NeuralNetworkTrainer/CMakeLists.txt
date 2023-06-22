
cmake_minimum_required (VERSION 3.21)

set(targetname NeuralNetworkTrainer)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")
file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "*.h")



add_library (DynaPlex_${targetname} STATIC)
add_library (DynaPlex::${targetname} ALIAS DynaPlex_${targetname})

##use CC_17 to avoid issues with PyTorch.
set_property(TARGET DynaPlex_${targetname} PROPERTY CXX_STANDARD 17)
set_property(TARGET DynaPlex_${targetname} PROPERTY CXX_STANDARD_REQUIRED true)
set_property(TARGET DynaPlex_${targetname} PROPERTY CXX_EXTENSIONS off)

set_target_properties(DynaPlex_${targetname} PROPERTIES 
	OUTPUT_NAME DynaPlex${targetname}
	EXPORT_NAME ${targetname}
)
target_sources(DynaPlex_${targetname} PUBLIC ${headers} PRIVATE ${private_headers} ${sources})


target_include_directories(DynaPlex_${targetname} 
PUBLIC 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(DynaPlex_${targetname} PUBLIC DynaPlex::Core)


if(${dynaplex_all_warnings})
target_compile_options(DynaPlex_${targetname} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
endif()

 

 
#Logic for including PyTorch Privately
if(Torch_FOUND)
   target_compile_definitions(DynaPlex_${targetname} PRIVATE Torch_available=1)
   target_link_libraries(DynaPlex_${targetname} PUBLIC "${TORCH_LIBRARIES}")
 if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET DynaPlex_${targetname}
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:DynaPlex_${targetname}>)

set_target_properties(DynaPlex_${targetname} PROPERTIES COMPILE_FLAGS "/wd4819 /wd4624")
set_target_properties(DynaPlex_${targetname} PROPERTIES COMPILE_FLAGS "/wd4702")
endif (MSVC)
endif()



if(${dynaplex_enable_pythonbindings})
#copy library (including .pyd) to subdirectory of python, so that the python scripts can find it
add_custom_command(TARGET DynaPlex_${targetname} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
       "${PROJECT_BINARY_DIR}/lib"
       "${PROJECT_SOURCE_DIR}/python/libs/")
       	   
#added to resolve issues when importing .pyd when also using pytorch.
if(Torch_FOUND)
 add_custom_command(TARGET DynaPlex_${targetname} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
       "${PROJECT_BINARY_DIR}/bin"
       "${PROJECT_SOURCE_DIR}/python/libs/")
endif()
endif()
