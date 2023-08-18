
cmake_minimum_required (VERSION 3.20)

set(targetname auto_tests)


add_executable (DP_${targetname})
add_executable (DynaPlex::${targetname} ALIAS DP_${targetname})


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")

add_test(NAME DynaPlexTests COMMAND DynaPlexTests)


target_sources(DP_${targetname} PRIVATE ${sources})


target_link_libraries(DP_${targetname} PRIVATE gtest gtest_main DynaPlex::DynaPlex )

if(${dynaplex_all_warnings})
target_compile_options(DP_${targetname} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
endif()



