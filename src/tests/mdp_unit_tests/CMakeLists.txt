﻿
cmake_minimum_required (VERSION 3.20)

set(targetname mdp_unit_tests)


add_executable (DP_${targetname})
add_executable (DynaPlex::${targetname} ALIAS DP_${targetname})


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "*.h")


add_test(NAME DP_${targetname} COMMAND DP_${targetname})

target_sources(DP_${targetname} PRIVATE ${sources} ${headers})



target_include_directories(DP_${targetname} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> )

target_link_libraries(DP_${targetname} PRIVATE gtest gtest_main DynaPlex::DynaPlex )

add_dependencies(DP_${targetname} DP_copy_model_config_files)

if(dynaplex_all_warnings)
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
endif()

if(DYNAPLEX_IO_ROOT_DIR)
target_compile_definitions(DP_${targetname} PRIVATE DYNAPLEX_IO_ROOT_DIR=\"${DYNAPLEX_IO_ROOT_DIR}\")
endif()


