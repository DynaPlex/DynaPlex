
cmake_minimum_required (VERSION 3.20)

set(targetname Example)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.h")


add_executable (${targetname} ${sources} ${headers})

target_include_directories(${targetname} PUBLIC include)

set_property(TARGET ${targetname} PROPERTY CXX_STANDARD 20)



target_link_libraries(${targetname} PRIVATE mdp_interface)

target_link_libraries(${targetname} PRIVATE mdp_opt)
