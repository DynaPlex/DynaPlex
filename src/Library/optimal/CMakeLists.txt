
cmake_minimum_required (VERSION 3.21)

set(targetname Optimal)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")
file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "*.h")


add_library (DynaPlex_${targetname} SHARED)
add_library (DynaPlex::${targetname} ALIAS DynaPlex_${targetname})

set_target_properties(DynaPlex_${targetname} PROPERTIES 
	OUTPUT_NAME DynaPlex${targetname}
	EXPORT_NAME ${targetname}
)


target_sources(DynaPlex_${targetname} PUBLIC ${headers} PRIVATE ${private_headers} ${sources})



target_include_directories(DynaPlex_${targetname} 
PUBLIC 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)


target_link_libraries(DynaPlex_${targetname} PUBLIC DynaPlex::MDP_Interfaces)


if(MSVC)
  target_compile_options(DynaPlex_${targetname} PRIVATE /W4)
else()
  target_compile_options(DynaPlex_${targetname} PRIVATE -Wall -Wextra -Wpedantic)
endif()
 
