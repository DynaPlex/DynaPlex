
#for succesfully linking DLL on windows.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
#for succesfully linking on linux to desired paths. 
set(CMAKE_SKIP_RPATH TRUE)
if(MSVC)
set(CMAKE_CXX_FLAGS "$CMAKE_CXX_FLAGS /EHsc")
endif()

set(stageDir ${CMAKE_CURRENT_BINARY_DIR})
include(GNUInstallDirs)
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${stageDir}/${CMAKE_INSTALL_BINDIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${stageDir}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${stageDir}/${CMAKE_INSTALL_LIBDIR})
endif()
#Modified because of loading issues of .pyd - to have all files in one location
#if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${stageDir}/${CMAKE_INSTALL_BINDIR})
#endif()
#if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${stageDir}/${CMAKE_INSTALL_BINDIR})
#endif()
