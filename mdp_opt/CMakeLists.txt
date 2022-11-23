cmake_minimum_required (VERSION 3.8)

set(targetname mdp_opt)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.h")


add_library (${targetname} STATIC)

target_sources(${targetname} PRIVATE ${headers} ${sources})

target_include_directories(${targetname} PUBLIC include)

set_target_properties(${targetname} PROPERTIES 
CXX_STANDARD 20 
LINKER_LANGUAGE CXX
)

target_link_libraries(${targetname} PUBLIC mdp_interface)