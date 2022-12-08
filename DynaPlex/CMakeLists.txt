cmake_minimum_required (VERSION 3.20)




add_subdirectory("mdp_interface")
add_subdirectory("mdp_opt")

#if(NOT WIN32 AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
#set(CMAKE_INSTALL_PREFIX "/opt/mycompany.com/${PROJECT_NAME}")
#endif()


#include(GNUInstallDirs)

#install(TARGETS mdp_opt mdp_interface
#EXPORT DynaPlex-export
#)
#install(EXPORT DynaPlex-export
#FILE DynaPlexTargets.cmake
#NAMESPACE DynaPlex::
#DESTINATION 
#lib/cmake/DynaPlex
#)
