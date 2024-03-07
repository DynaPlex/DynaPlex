cmake_minimum_required (VERSION 3.21)
set(targetname Bindings)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")

pybind11_add_module(DP_${targetname})

set_target_properties(DP_${targetname} PROPERTIES 
	OUTPUT_NAME DP_${targetname}
)
target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE  ${sources})


target_compile_definitions(DP_${targetname} PRIVATE DP_PYBIND_SUPPORT=1)

target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)



if(dynaplex_all_warnings)
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3>  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
endif()

if(Torch_FOUND)
   target_compile_definitions(DP_${targetname} PRIVATE DP_TORCH_AVAILABLE=1)
   target_link_libraries(DP_${targetname} PUBLIC "${TORCH_LIBRARIES}")
endif()

if(MPI_FOUND)
target_compile_definitions(DP_${targetname} PRIVATE DP_MPI_AVAILABLE=1)
target_link_libraries(DP_${targetname} PUBLIC MPI::MPI_CXX)
endif()

 add_custom_command(TARGET DP_${targetname} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DP_${targetname}>
                   ${PROJECT_SOURCE_DIR}/python/dp/
                   COMMENT "Copying python extension to python/dp/")