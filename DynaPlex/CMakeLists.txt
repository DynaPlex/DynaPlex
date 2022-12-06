cmake_minimum_required (VERSION 3.20)




add_subdirectory("mdp_interface")
add_subdirectory("mdp_opt")

if(NOT WIN32 AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
set(CMAKE_INSTALL_PREFIX "/opt/mycompany.com/${PROJECT_NAME}")
endif()



