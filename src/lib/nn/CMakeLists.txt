
cmake_minimum_required (VERSION 3.21)

set(targetname NN)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")
file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "*.h")

add_library (DP_${targetname} OBJECT)
add_library (DynaPlex::${targetname} ALIAS DP_${targetname})

target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${private_headers} ${sources})
target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )

if(${dynaplex_enable_pythonbindings})
target_sources(DP_Bindings PUBLIC ${headers} PRIVATE ${private_headers} ${sources} )
target_include_directories(DP_Bindings PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )
endif()

target_link_libraries(DP_${targetname} PUBLIC DynaPlex::Core)


if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3>  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra> )
endif()
 
#Logic for including PyTorch Privately
if(Torch_FOUND)
   target_compile_definitions(DP_${targetname} PRIVATE Torch_available=1)
   target_link_libraries(DP_${targetname} PUBLIC "${TORCH_LIBRARIES}")
endif()