#pragma once

#include <stdexcept>
#include <string>

namespace DynaPlex {

    class Error : public std::runtime_error {
    public:
        explicit Error(const std::string& message)
            : std::runtime_error(std::string{ "DynaPlex: " } + message) {}

        explicit Error(const char* message) 
            : std::runtime_error(std::string{ "DynaPlex: " } + message) {}
    };

} // namespace DynaPlex
