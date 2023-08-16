cmake_minimum_required (VERSION 3.21)
set(targetname Bindings)


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")
file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "*.h")


pybind11_add_module(DP_${targetname} MODULE)



set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD 20)
set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD_REQUIRED true)
set_property(TARGET DP_${targetname} PROPERTY CXX_EXTENSIONS off)

set_property(TARGET DP_${targetname} PROPERTY CXX_VISIBILITY_PRESET default)
set_property(TARGET DP_${targetname} PROPERTY VISIBILITY_INLINES_HIDDEN off)
set_property(TARGET DP_${targetname} PROPERTY INTERPROCEDURAL_OPTIMIZATION OFF)

set_property(TARGET DP_${targetname} PROPERTY POSITION_INDEPENDENT_CODE ON)

set_target_properties(DP_${targetname} PROPERTIES 
	OUTPUT_NAME DP_${targetname}
	EXPORT_NAME ${targetname}
)
target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${private_headers} ${sources})

target_include_directories(DP_${targetname} 
PUBLIC 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(DP_${targetname} PRIVATE DynaPlex::DynaPlex)

if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
endif()


#Logic for including PyTorch Privately
if(Torch_FOUND)
   target_compile_definitions(DP_${targetname} PRIVATE Torch_available=1)
   target_link_libraries(DP_${targetname} PUBLIC "${TORCH_LIBRARIES}")
endif()

 add_custom_command(TARGET DP_${targetname} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/python/dp_lib/libs/"
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DP_${targetname}>
                   ${PROJECT_SOURCE_DIR}/python/dp_lib/libs/
                   COMMENT "Copying python extension to python/dp_lib/libs/")