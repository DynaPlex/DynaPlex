cmake_minimum_required (VERSION 3.21)
set(targetname MDP_Interfaces)

file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")

add_library (DynaPlex_${targetname} INTERFACE)
add_library (DynaPlex::${targetname} ALIAS DynaPlex_${targetname})

target_sources(DynaPlex_${targetname} INTERFACE ${headers})


target_include_directories(DynaPlex_${targetname}
INTERFACE 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)



