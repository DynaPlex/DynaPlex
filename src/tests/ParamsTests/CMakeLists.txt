
cmake_minimum_required (VERSION 3.20)

set(targetname ParamsTests)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.h")


add_executable (DP_${targetname} ${sources})
set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD 20)
set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD_REQUIRED true)
set_property(TARGET DP_${targetname} PROPERTY CXX_EXTENSIONS off)



if(${dynaplex_enable_pythonbindings})
set_property(TARGET DP_${targetname} PROPERTY EXCLUDE_FROM_ALL TRUE)
else()
set_property(TARGET DP_${targetname} PROPERTY EXCLUDE_FROM_ALL FALSE)
endif()



set_target_properties(DP_${targetname} PROPERTIES
    BUILD_RPATH "${stageDir}/${CMAKE_INSTALL_LIBDIR}")


set_target_properties(DP_${targetname} PROPERTIES 
	OUTPUT_NAME DynaPlex${targetname}
	EXPORT_NAME ${targetname}
)

if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
endif()

 
target_include_directories(DP_${targetname} 
PUBLIC 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(DP_${targetname} PRIVATE DynaPlex::DynaPlex)

