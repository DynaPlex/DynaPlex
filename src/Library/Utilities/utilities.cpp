#include "dynaplex/utilities.h"
#include <iostream>
#include <filesystem>


namespace fs = std::filesystem;



std::string DynaPlex::Utilities::GetOutputLocation(const std::string filename)
{
#ifdef _WIN32
    const char* desktopFolder = getenv("USERPROFILE");
    
    
    if (desktopFolder) {
        fs::path desktopPath(desktopFolder);
        desktopPath /= "Desktop";
        fs::path addedpath = desktopPath / "DynaPlex";
        fs::create_directory(addedpath);
        addedpath /= filename;
        return addedpath.string();
    }
#else
    const char* desktopFolder = getenv("HOME");
    if (desktopFolder) {
        fs::path desktopPath(desktopFolder);
        desktopPath.append("Desktop");
        return desktopPath.string();
    }
#endif

    std::cerr << "Error: Unable to get the desktop path." << std::endl;
    return std::string();
}
