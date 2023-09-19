
cmake_minimum_required (VERSION 3.21)

set(targetname Models)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")



if(dynaplex_enable_pythonbindings)
target_sources(DP_Bindings PUBLIC ${headers} PRIVATE ${sources} )
target_include_directories(DP_Bindings PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
else()
add_library (DP_${targetname} STATIC)
add_library (DynaPlex::${targetname} ALIAS DP_${targetname})
set_target_properties(DP_${targetname} PROPERTIES OUTPUT_NAME DynaPlex_${targetname} EXPORT_NAME ${targetname})
target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${sources})
target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Core)
if(dynaplex_all_warnings)
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
endif()


    if(DYNAPLEX_IO_ROOT_DIR)
       if(NOT EXISTS ${DYNAPLEX_IO_ROOT_DIR})
            message(FATAL_ERROR "DYNAPLEX_IO_ROOT_DIR is set to a non-existent directory: ${DYNAPLEX_IO_ROOT_DIR}")
       endif()    
       # Ensure the destination directory structure exists.
       file(MAKE_DIRECTORY ${DYNAPLEX_IO_ROOT_DIR}/IO_DynaPlex/defaults)
       # Iterate through subdirectories in 'models' and copy JSON files.
       file(GLOB_RECURSE MODEL_JSON_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/models" "${CMAKE_CURRENT_SOURCE_DIR}/models/*/*.json")
       foreach(JSON_FILE ${MODEL_JSON_FILES})
         get_filename_component(JSON_DIR ${JSON_FILE} DIRECTORY)
         set(JSON_DEST "${DYNAPLEX_IO_ROOT_DIR}/IO_DynaPlex/defaults/${JSON_DIR}")
         add_custom_command(TARGET DP_${targetname} POST_BUILD
           COMMAND ${CMAKE_COMMAND} -E make_directory ${JSON_DEST}
           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/models/${JSON_FILE} ${JSON_DEST}
           COMMENT "Copying ${JSON_FILE} to ${JSON_DEST}")
       endforeach()
    else()
        message(FATAL_ERROR "DYNAPLEX_IO_ROOT_DIR not provided")
    endif()
endif() 