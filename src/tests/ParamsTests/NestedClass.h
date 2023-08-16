#pragma once

#include <string>
#include "dynaplex/params.h"

class NestedClass {
public:
    NestedClass(DynaPlex::Params& params);
    NestedClass() {}
    void Print() const;

private:
    std::string Id;
    double Size;
};

