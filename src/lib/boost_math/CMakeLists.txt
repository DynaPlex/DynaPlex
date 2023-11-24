#cmakelists for linking only the include library. 
add_library(boost_math INTERFACE)

add_library(Boost::math ALIAS boost_math)

target_include_directories(boost_math INTERFACE include)
target_compile_definitions(boost_math INTERFACE BOOST_MATH_STANDALONE=1)

