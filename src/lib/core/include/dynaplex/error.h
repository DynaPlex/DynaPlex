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

    class NotImplementedError : public Error {
    public:
        explicit NotImplementedError(const std::string& message = "Functionality not yet implemented")
            : Error(message) {}

        explicit NotImplementedError(const char* message)
            : Error(message) {}
    };


} // namespace DynaPlex
