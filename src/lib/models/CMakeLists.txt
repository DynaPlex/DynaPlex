
cmake_minimum_required (VERSION 3.21)

set(targetname Models)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")



if(dynaplex_enable_pythonbindings)
   target_sources(DP_Bindings PUBLIC ${headers} PRIVATE ${sources} )
   target_link_libraries(DP_Bindings PRIVATE Boost::math)
   target_include_directories(DP_Bindings PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
else(dynaplex_enable_pythonbindings)
   add_library (DP_${targetname} STATIC)
   add_library (DynaPlex::${targetname} ALIAS DP_${targetname})
   set_target_properties(DP_${targetname} PROPERTIES OUTPUT_NAME DynaPlex_${targetname} EXPORT_NAME ${targetname})
   target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${sources})
   target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
   target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Core Boost::math)
   if(dynaplex_all_warnings)
      target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
   endif()
   install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
   install(TARGETS DP_${targetname} DESTINATION bin)

   if(DYNAPLEX_IO_ROOT_DIR)
     if(NOT EXISTS ${DYNAPLEX_IO_ROOT_DIR})
         message(FATAL_ERROR "DYNAPLEX_IO_ROOT_DIR is set to a non-existent directory: ${DYNAPLEX_IO_ROOT_DIR}")
     endif()
    
      # Ensure the destination directory structure exists.
      file(MAKE_DIRECTORY ${DYNAPLEX_IO_ROOT_DIR}/IO_DynaPlex/mdp_config_examples)

      # Iterate through subdirectories in 'models' and find JSON files.
      file(GLOB_RECURSE MODEL_JSON_FILES CONFIGURE_DEPENDS  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/models" "${CMAKE_CURRENT_SOURCE_DIR}/models/*/*.json")
      # Create a list to hold all destination JSON files
      set(ALL_JSON_DESTS)

      foreach(JSON_FILE ${MODEL_JSON_FILES})
         get_filename_component(JSON_DIR ${JSON_FILE} DIRECTORY)
         set(JSON_DEST "${DYNAPLEX_IO_ROOT_DIR}/IO_DynaPlex/mdp_config_examples/${JSON_FILE}")
    
         add_custom_command(
             OUTPUT ${JSON_DEST}
             COMMAND ${CMAKE_COMMAND} -E make_directory ${JSON_DIR}
             COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/models/${JSON_FILE} ${JSON_DEST}
             DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/models/${JSON_FILE}
             COMMENT "Copying ${JSON_FILE} to ${JSON_DEST}"
         )
         # Append the destination to the list
         list(APPEND ALL_JSON_DESTS ${JSON_DEST})
      endforeach()

      # Create a custom target that depends on all copied JSON files
      add_custom_target(DP_copy_model_config_files ALL DEPENDS ${ALL_JSON_DESTS})
      else(DYNAPLEX_IO_ROOT_DIR)
         message(STATUS "DYNAPLEX_IO_ROOT_DIR not provided")
    endif(DYNAPLEX_IO_ROOT_DIR)


endif(dynaplex_enable_pythonbindings) 