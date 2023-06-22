
cmake_minimum_required (VERSION 3.20)

set(targetname ParamsTests)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.h")


add_executable (DynaPlex_${targetname} ${sources})
set_property(TARGET DynaPlex_${targetname} PROPERTY CXX_STANDARD 20)
set_property(TARGET DynaPlex_${targetname} PROPERTY CXX_STANDARD_REQUIRED true)
set_property(TARGET DynaPlex_${targetname} PROPERTY CXX_EXTENSIONS off)
set_property(TARGET DynaPlex_${targetname} PROPERTY EXCLUDE_FROM_ALL TRUE)




set_target_properties(DynaPlex_${targetname} PROPERTIES 
	OUTPUT_NAME DynaPlex${targetname}
	EXPORT_NAME ${targetname}
)

if(${dynaplex_all_warnings})
target_compile_options(DynaPlex_${targetname} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
endif()

 

target_include_directories(DynaPlex_${targetname} PUBLIC include)

target_link_libraries(DynaPlex_${targetname} PRIVATE DynaPlex::DynaPlex)

