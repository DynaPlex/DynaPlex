cmake_minimum_required (VERSION 3.21)
set(targetname Bindings)


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/dynaplex/*.h")
file(GLOB_RECURSE private_headers CONFIGURE_DEPENDS "*.h")


pybind11_add_module(DP_${targetname})



set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD 20)
set_property(TARGET DP_${targetname} PROPERTY CXX_STANDARD_REQUIRED true)
set_property(TARGET DP_${targetname} PROPERTY CXX_EXTENSIONS off)

set_target_properties(DP_${targetname} PROPERTIES 
	OUTPUT_NAME DP_Bindings
	EXPORT_NAME ${targetname}
)
target_sources(DP_${targetname} PUBLIC ${headers} PRIVATE ${private_headers} ${sources})

target_include_directories(DP_${targetname} 
PUBLIC 
$<INSTALL_INTERFACE:include>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(DP_${targetname} PUBLIC DynaPlex::DynaPlex)


if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
endif()




 add_custom_command(TARGET DP_${targetname} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/python/libs/"
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DP_${targetname}>
                   ${PROJECT_SOURCE_DIR}/python/libs/
                   COMMENT "Copying python extension to python/libs/")