
cmake_minimum_required (VERSION 3.20)

set(targetname Example)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.h")


add_executable (DynaPlex_${targetname} ${sources} ${headers})

target_include_directories(DynaPlex_${targetname} PUBLIC include)

set_property(TARGET DynaPlex_${targetname} PROPERTY CXX_STANDARD 20)



target_link_libraries(DynaPlex_${targetname} PRIVATE DynaPlex::MDP_Interfaces)

target_link_libraries(DynaPlex_${targetname} PRIVATE DynaPlex::Optimal)
