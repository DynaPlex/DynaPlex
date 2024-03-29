
cmake_minimum_required (VERSION 3.21)

set(targetname Utilities)

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
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
install(TARGETS DP_${targetname} DESTINATION bin)
if(dynaplex_all_warnings)
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3>  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra> )
endif() 
endif()

