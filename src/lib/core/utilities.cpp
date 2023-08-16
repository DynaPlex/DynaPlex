#include "dynaplex/utilities.h"
#include <iostream>

int DynaPlex::Utilities::mult_(int a, int b) {
    return a * b;
}

std::string DynaPlex::Utilities::GetOutputLocation(const std::string filename)
{   
    return std::string(filename);
}
