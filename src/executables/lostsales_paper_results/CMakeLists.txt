
cmake_minimum_required (VERSION 3.20)

set(targetname lostsales_paper_results)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")

add_executable (${targetname})

set_property(TARGET ${targetname} PROPERTY EXCLUDE_FROM_ALL TRUE)

target_sources(${targetname} PRIVATE ${headers} ${sources})
target_include_directories(${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )

target_link_libraries(${targetname} PRIVATE DynaPlex::DynaPlex )



