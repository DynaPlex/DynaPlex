
cmake_minimum_required (VERSION 3.20)

set(targetname memory_checks)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")

add_executable (DP_${targetname})
add_executable (DynaPlex::${targetname} ALIAS DP_${targetname})

set_property(TARGET DP_${targetname} PROPERTY EXCLUDE_FROM_ALL TRUE)

target_sources(DP_${targetname} PRIVATE ${headers} ${sources})
target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )

target_link_libraries(DP_${targetname} PRIVATE DynaPlex::DynaPlex )

if(dynaplex_all_warnings)
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
endif()



