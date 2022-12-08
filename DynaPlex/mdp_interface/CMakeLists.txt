
cmake_minimum_required (VERSION 3.20)

set(targetname mdp_interface)


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.h")


add_library (${targetname} INTERFACE)

target_sources(${targetname} INTERFACE ${headers})

target_include_directories(${targetname} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(${targetname} PROPERTIES 
CXX_STANDARD 20 
LINKER_LANGUAGE CXX
)

target_include_directories(${targetname} 
INTERFACE 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

