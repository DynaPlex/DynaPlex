
cmake_minimum_required (VERSION 3.21)

set(targetname Optimal)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")
file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "*.h")


add_library (DP_${targetname} OBJECT)
add_library (DynaPlex::${targetname} ALIAS DP_${targetname})

set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD 20)
set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD_REQUIRED true)
set_property(TARGET DP_${targetname} PROPERTY CXX_EXTENSIONS off)

set_property(TARGET DP_${targetname} PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET DP_${targetname} PROPERTY CXX_VISIBILITY_PRESET default)
set_property(TARGET DP_${targetname} PROPERTY VISIBILITY_INLINES_HIDDEN 0)

set_target_properties(DP_${targetname} PROPERTIES 
	OUTPUT_NAME DynaPlex${targetname}
	EXPORT_NAME ${targetname}
)
target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${private_headers} ${sources})


if(${dynaplex_enable_pythonbindings})
target_sources(DP_Bindings PUBLIC ${headers} PRIVATE ${private_headers} ${sources} )
target_include_directories(DP_Bindings PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
endif()


target_include_directories(DP_${targetname} 
PUBLIC 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Core)


if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
endif()

 